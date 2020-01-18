#include "Game/Feature.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

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
    DataUtils::ValidateXmlElement(elem, "feature", "", "name,position");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    const auto definitionName = DataUtils::ParseXmlAttribute(elem, "name", "");
    _tile_def = TileDefinition::GetTileDefinitionByName(definitionName);
    _isSolid = _tile_def->is_solid;
    _isOpaque = _tile_def->is_opaque;
    _isVisible = _tile_def->is_visible;
    sprite = _tile_def->GetSprite();
    SetPosition(DataUtils::ParseXmlAttribute(elem, "position", IntVector2::ZERO));
    return true;
}

void Feature::Update(TimeUtils::FPSeconds deltaSeconds) {
    Entity::Update(deltaSeconds);
}

void Feature::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
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

    const float z = static_cast<float>(layer_index);
    verts.push_back(Vertex3D(Vector3(vert_bl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tl));
    verts.push_back(Vertex3D(Vector3(vert_tr, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tr));
    verts.push_back(Vertex3D(Vector3(vert_br, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_br));

    const auto v_s = verts.size();
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 3u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 1u);
}

bool Feature::ToggleSolid() noexcept {
    _isSolid = !_isSolid;
    return _isSolid;
}

bool Feature::IsSolid() const noexcept {
    return _isSolid;
}

bool Feature::ToggleOpaque() noexcept {
    _isOpaque = !_isOpaque;
    return _isOpaque;
}

bool Feature::IsOpaque() const noexcept {
    return _isOpaque;
}

bool Feature::ToggleVisible() noexcept {
    _isVisible = !_isVisible;
    return _isVisible;
}

bool Feature::IsVisible() const {
    return _isVisible;
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

void Feature::ResolveAttack(Entity& /*attacker*/, Entity& /*defender*/) {
    /* DO NOTHING */
}
