#include "Game/Feature.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

#include <sstream>

Feature* Feature::CreateFeature(Map* map, const XMLElement& elem) {
    auto new_feature = std::make_unique<Feature>(map, elem);
    std::string new_feature_name = new_feature->name;
    auto* ptr = new_feature.get();
    s_registry.emplace(new_feature_name, std::move(new_feature));
    return ptr;
}

FeatureInstance Feature::CreateInstanceFromFeature(const Feature* feature) noexcept {
    return CreateInstanceFromFeatureAt(feature, feature->GetPosition());
}

FeatureInstance Feature::CreateInstanceFromFeatureAt(const Feature* feature, const IntVector2& position) noexcept {
    if(feature == nullptr) {
        return {};
    }
    if(const auto layer = feature->layer; layer != nullptr) {
        const auto tileCount = feature->layer->tileDimensions.x * feature->layer->tileDimensions.y;
        if(const auto index = feature->layer->GetTileIndex(position.x, position.y); index >= tileCount) {
            return {};
        } else {
            FeatureInstance inst{};
            inst.feature = feature;
            inst.layer_index = feature->layer->z_index;
            inst.index = index;
            inst.states = feature->_states;
            inst.current_state = std::begin(inst.states);
            return inst;
        }
    }
    return {};
}

std::optional<FeatureInstance> Feature::CreateInstanceFromFeatureByName(std::string name) noexcept {
    if(auto* f = GetFeatureByName(name); f != nullptr) {
        return CreateInstanceFromFeature(f);
    }
    return std::nullopt;
}

std::optional<FeatureInstance> Feature::CreateInstanceFromFeatureByNameAt(std::string name, const IntVector2& position) noexcept {
    if(auto* f = GetFeatureByName(name); f != nullptr) {
        return CreateInstanceFromFeatureAt(f, position);
    }
    return std::nullopt;
}

void Feature::ClearFeatureRegistry() {
    s_registry.clear();
}

Feature* Feature::GetFeatureByName(const std::string& name) {
    auto found_iter = s_registry.find(name);
    if(found_iter != std::end(s_registry)) {
        return found_iter->second.get();
    }
    return nullptr;
}

Feature* Feature::GetFeatureByGlyph(const char glyph) {
    for(const auto& feature : s_registry) {
        if(TileDefinition::GetTileDefinitionByName(feature.second->parent_tile->GetType())->glyph == glyph) {
            return feature.second.get();
        }
    }
    return nullptr;
}

Feature::Feature(Map* map, const XMLElement& elem) noexcept
    : Entity()
{
    this->map = map;
    this->layer = this->map->GetLayer(0);
    GUARANTEE_OR_DIE(LoadFromXml(elem), "Feature failed to load.");
    OnFight.Subscribe_method(this, &Feature::ResolveAttack);
    OnDamage.Subscribe_method(this, &Feature::ApplyDamage);
    OnMiss.Subscribe_method(this, &Feature::AttackerMissed);
    OnDestroy.Subscribe_method(this, &Feature::OnDestroyed);
}

//TODO: Refactor for instancing
bool Feature::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "feature", "", "name", "state", "position,initialState");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);

    const auto featureName = DataUtils::ParseXmlAttribute(elem, "name", std::string{});
    std::string definitionName = featureName;
    if(const auto state_count = DataUtils::GetChildElementCount(elem, "state"); state_count > 0) {
        DataUtils::ForEachChildElement(elem, "state", [&definitionName, &featureName, this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "state", "", "name");
            const auto cur_name = DataUtils::ParseXmlAttribute(elem, "name", std::string{});
            definitionName = featureName + "." + cur_name;
            _states.push_back(definitionName);
            });
        const auto has_initialState = DataUtils::HasAttribute(elem, "initialState");
        const auto attr = elem.Attribute("initialState");
        std::string initialState = std::string(attr ? attr : "");
        const auto valid_initialState = has_initialState && !initialState.empty();
        if(!valid_initialState) {
            auto* logger = ServiceLocator::get<IFileLoggerService>();
            const auto iter = std::begin(_states);
            const auto first_state_name = iter->substr(iter->find_last_of('.') + 1);
            logger->LogLineAndFlush(std::format("Feature initialState attribute for feature \"{}\" is empty or missing. Defaulting to first state: {}.", featureName, first_state_name));
        }
        initialState = featureName + "." + initialState;
        if(auto found = std::find(std::begin(_states), std::end(_states), initialState); found != std::end(_states)) {
            _current_state = found;
        } else {
            _current_state = std::begin(_states);
        }
        definitionName = (*_current_state);
    } else {
        _states.push_back(definitionName);
        _current_state = std::begin(_states);
    }

    if(auto* tile_def = TileDefinition::GetTileDefinitionByName(definitionName); tile_def != nullptr) {
        sprite = tile_def->GetSprite();
        _light_value = tile_def->light;
        _self_illumination = tile_def->self_illumination;
    }

    if(DataUtils::HasAttribute(elem, "position")) {
        SetPosition(DataUtils::ParseXmlAttribute(elem, "position", IntVector2::Zero));
        parent_tile = map->GetTile(IntVector3{GetPosition(), layer->z_index});
    }

    return true;
}

bool Feature::IsOpaque() const noexcept {
    return TileDefinition::GetTileDefinitionByName(*_current_state)->is_opaque;
}

bool Feature::IsSolid() const noexcept {
    return TileDefinition::GetTileDefinitionByName(*_current_state)->is_solid;
}

bool Feature::IsVisible() const noexcept {
    return TileDefinition::GetTileDefinitionByName(*_current_state)->is_visible;
}

bool Feature::IsInvisible() const noexcept {
    return !IsVisible();
}

void Feature::SetPosition(const IntVector2& position) {
    auto cur_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    cur_tile->feature = nullptr;
    Entity::SetPosition(position);
    auto next_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    next_tile->feature = this;
    tile = next_tile;
}

void Feature::SetState(const std::string& stateName) {
    if(auto* new_def = TileDefinition::GetTileDefinitionByName(name + "." + stateName)) {
        TileInfo ti{layer, layer->GetTileIndex(_position.x, _position.y)};
        ti.SetLightDirty();
        sprite = new_def->GetSprite();
        return;
    }
    DebuggerPrintf("Attempting to set Feature to invalid state: %s\n", stateName.c_str());
}

void Feature::ResolveAttack(Entity& attacker, Entity& defender) {
    auto* defenderAsFeature = dynamic_cast<Feature*>(&defender);
    if(this == defenderAsFeature) {
        if(auto key = attacker.inventory.HasItem("key")) {
            attacker.inventory.RemoveItem(key);
            defenderAsFeature->SetState("open");
        }
    }
}

void Feature::ApplyDamage(DamageType type, long amount, bool crit) {
    if(type == DamageType::Physical) {
        auto stats = this->GetStats();
        if(const auto newHealth = stats.AdjustStat(StatsID::Health, -amount); crit || newHealth <= 0L) {
            OnDestroy.Trigger();
        }
    }
}

void Feature::AttackerMissed() {
    /* DO NOTHING */
}

void Feature::OnDestroyed() {
    auto info = FeatureInfo{ layer, parent_tile->GetIndexFromCoords() };
    info.SetState("open");
}

FeatureInstance Feature::CreateInstance() const noexcept {
    return CreateInstanceAt(GetPosition());
}

FeatureInstance Feature::CreateInstanceAt(const IntVector2& position) const noexcept {
    FeatureInstance inst{};
    inst.feature = this;
    inst.layer = layer;
    inst.index = layer->GetTileIndex(position.x, position.y);
    //inst.states.push_back();
    inst.current_state = std::begin(inst.states);
    return inst;
}

void Feature::CalculateLightValue() noexcept {
    SetLightValue(_light_value);
}

Tile* FeatureInstance::GetParentTile() const noexcept {
    if(layer || feature) {
        return nullptr;
    }
    return layer->GetTile(index);
}

void FeatureInstance::AddState() noexcept {
    /* DO NOTHING */
}

void FeatureInstance::SetState(std::vector<std::string>::iterator iterator) noexcept {
    current_state = iterator;
}

void FeatureInstance::SetStatebyName(const std::string& name) noexcept {
    if(auto found = std::find(std::begin(states), std::end(states), name); found != std::end(states)) {
        SetState(found);
    }
}

bool FeatureInfo::HasState(std::string stateName) const noexcept {
    if(!HasStates()) {
        return false;
    }
    auto* feature = layer->GetTile(index)->feature;
    return std::find(std::cbegin(feature->_states), std::cend(feature->_states), std::format("{}.{}", feature->name, stateName)) != std::cend(feature->_states);
}

bool FeatureInfo::HasStates() const noexcept {
    if(layer == nullptr) {
        return false;
    }
    if(auto* tile = layer->GetTile(index); tile == nullptr) {
        return false;
    } else {
        if(auto* feature = tile->feature; feature == nullptr) {
            return false;
        } else {
            return !feature->_states.empty();
        }
    }
}

std::vector<std::string> FeatureInfo::GetStates() const noexcept {
    if(layer == nullptr) {
        return {};
    }
    if(auto* tile = layer->GetTile(index)) {
        if(auto* feature = tile->feature) {
            return feature->_states;
        }
    }
    return {};
}

bool FeatureInfo::SetState(std::string newState) noexcept {
    if(!HasState(newState)) {
        return false;
    }
    layer->GetTile(index)->feature->SetState(newState);
    return true;
}

std::string FeatureInfo::GetCurrentState() const noexcept {
    if(!HasStates()) {
        return {};
    }
    if(auto* tile = layer->GetTile(index)) {
        if(auto* feature = tile->feature) {
            return *(feature->_current_state);
        }
    }
    return {};
}
