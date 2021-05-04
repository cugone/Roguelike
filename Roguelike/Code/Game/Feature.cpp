#include "Game/Feature.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

#include <sstream>

std::map<std::string, std::unique_ptr<Feature>> Feature::s_registry{};


Feature* Feature::CreateFeature(Map* map, const XMLElement& elem) {
    auto new_feature = std::make_unique<Feature>(map, elem);
    auto new_feature_name = new_feature->name;
    s_registry.try_emplace(new_feature_name, std::move(new_feature));
    return new_feature.get();
}

void Feature::ClearFeatureRegistry() {
    s_registry.clear();
}

Feature::Feature(Map* map, const XMLElement& elem) noexcept
    : Entity()
{
    this->map = map;
    this->layer = this->map->GetLayer(0);
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Feature failed to load.");
    }
    OnFight.Subscribe_method(this, &Feature::ResolveAttack);
}

bool Feature::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "feature", "", "name", "state", "position,initialState");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);

    const auto featureName = DataUtils::ParseXmlAttribute(elem, "name", "");
    std::string definitionName = featureName;
    const auto state_count = DataUtils::GetChildElementCount(elem, "state");
    
    if(!state_count) {
        _tile_def = TileDefinition::GetTileDefinitionByName(definitionName);
    } else {
        DataUtils::ForEachChildElement(elem, "state", [&definitionName, &featureName, this](const XMLElement& elem) {
            const auto cur_name = DataUtils::ParseXmlAttribute(elem, "name", "");
            definitionName = featureName + "." + cur_name;
            _states.push_back(definitionName);
        });
        const auto has_initialState = DataUtils::HasAttribute(elem, "initialState");
        const auto attr = elem.Attribute("initialState");
        std::string initialState = std::string(attr ? attr : "");
        const auto valid_initialState = has_initialState && !initialState.empty();
        if(!valid_initialState) {
            DebuggerPrintf("Feature initialState attribute is empty or missing. Defaulting to first state.");
        }
        initialState = featureName + "." + initialState;
        _tile_def = TileDefinition::GetTileDefinitionByName(valid_initialState ? initialState : _states[0]);
    }
    
    sprite = _tile_def->GetSprite();
    
    if(DataUtils::HasAttribute(elem, "position")) {
        SetPosition(DataUtils::ParseXmlAttribute(elem, "position", IntVector2::ZERO));
    }
    
    return true;
}

bool Feature::IsTransparent() const noexcept {
    return _tile_def->is_transparent;
}

bool Feature::IsSolid() const noexcept {
    return _tile_def->is_solid;
}

bool Feature::IsOpaque() const noexcept {
    return _tile_def->is_opaque;
}

bool Feature::IsVisible() const {
    return _tile_def->is_visible;
}

bool Feature::IsNotVisible() const {
    return !IsVisible();
}

bool Feature::IsInvisible() const {
    return IsNotVisible();
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
        _tile_def = new_def;
        sprite = _tile_def->GetSprite();
        return;
    }
    DebuggerPrintf("Attempting ot set Feature to invalid state: %s\n", stateName.c_str());
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
