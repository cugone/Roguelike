#include "Game/Feature.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

#include <sstream>

std::map<std::string, std::unique_ptr<Feature>> Feature::s_registry{};


Feature* Feature::CreateFeature(Map* map, const XMLElement& elem) {
    auto new_feature = std::make_unique<Feature>(map, elem);
    std::string new_feature_name = new_feature->name;
    if(auto&& [where, inserted] = s_registry.try_emplace(new_feature_name, std::move(new_feature)); inserted) {
        return where->second.get();
    }
    return nullptr;
}

FeatureInstance Feature::CreateInstanceFromFeature(Feature* feature) noexcept {
    return CreateInstanceFromFeatureAt(feature, IntVector2::ZERO);
}

FeatureInstance Feature::CreateInstanceFromFeatureAt(Feature* feature, const IntVector2& position) noexcept {
    if(feature == nullptr) {
        return {};
    }
    if(const auto layer = feature->layer; layer != nullptr) {
        const auto tileCount = feature->layer->tileDimensions.x * feature->layer->tileDimensions.y;
        if(const auto index = feature->layer->GetTileIndex(position.x, position.y); index >= tileCount) {
            return {};
        }
        FeatureInstance inst{};
        inst.feature = feature;
        inst.layer = feature->layer;
    }
    return {};
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
}

//TODO: Refactor for instancing
bool Feature::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "feature", "", "name", "state", "position,initialState");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);

    const auto featureName = DataUtils::ParseXmlAttribute(elem, "name", "");
    std::string definitionName = featureName;
    if(const auto state_count = DataUtils::GetChildElementCount(elem, "state"); state_count > 0) {
        DataUtils::ForEachChildElement(elem, "state", [&definitionName, &featureName, this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "state", "", "name");
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
        SetPosition(DataUtils::ParseXmlAttribute(elem, "position", IntVector2::ZERO));
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

void Feature::AddVerts() noexcept {
    AddVertsForSelf();
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

void Feature::AddVertsForSelf() noexcept {
    if(!sprite || IsInvisible()) {
        return;
    }
    const auto& coords = sprite->GetCurrentTexCoords();

    const auto vert_left = _position.x + 0.0f;
    const auto vert_right = _position.x + 1.0f;
    const auto vert_top = _position.y + 0.0f;
    const auto vert_bottom = _position.y + 1.0f;

    const auto vert_bl = Vector2(vert_left, vert_bottom);
    const auto vert_tl = Vector2(vert_left, vert_top);
    const auto vert_tr = Vector2(vert_right, vert_top);
    const auto vert_br = Vector2(vert_right, vert_bottom);

    const auto tx_left = coords.mins.x;
    const auto tx_right = coords.maxs.x;
    const auto tx_top = coords.mins.y;
    const auto tx_bottom = coords.maxs.y;

    const auto tx_bl = Vector2(tx_left, tx_bottom);
    const auto tx_tl = Vector2(tx_left, tx_top);
    const auto tx_tr = Vector2(tx_right, tx_top);
    const auto tx_br = Vector2(tx_right, tx_bottom);

    const float z = static_cast<float>(layer->z_index);
    const Rgba layer_color = layer->color;

    auto& builder = layer->GetMeshBuilder();
    const auto newColor = [&]() {
        auto clr = layer_color != color && color != Rgba::White ? color : layer_color;
        clr.ScaleRGB(MathUtils::RangeMap(static_cast<float>(GetLightValue()), static_cast<float>(min_light_value), static_cast<float>(max_light_value), min_light_scale, max_light_scale));
        return clr;
    }(); //IIIL
    const auto normal = -Vector3::Z_AXIS;

    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(newColor);
    builder.SetNormal(normal);

    builder.SetUV(tx_bl);
    builder.AddVertex(Vector3{vert_bl, z});

    builder.SetUV(tx_tl);
    builder.AddVertex(Vector3{vert_tl, z});

    builder.SetUV(tx_tr);
    builder.AddVertex(Vector3{vert_tr, z});

    builder.SetUV(tx_br);
    builder.AddVertex(Vector3{vert_br, z});

    builder.AddIndicies(Mesh::Builder::Primitive::Quad);
    builder.End(sprite->GetMaterial());

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

