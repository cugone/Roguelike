#include "Game/Feature.hpp"

#include "Game/Map.hpp"

Feature::Feature(const XMLElement& elem) noexcept
    : Entity(elem)
{
    /* DO NOTHING */
}

void Feature::Update(TimeUtils::FPSeconds deltaSeconds) {
    Entity::Update(deltaSeconds);
}

void Feature::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {
    Entity::Render(verts, ibo, layer_color, layer_index);
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

void Feature::SetPosition(const IntVector2& position) {
    auto cur_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    cur_tile->feature = nullptr;
    Entity::SetPosition(position);
    auto next_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    next_tile->feature = this;
    tile = next_tile;
}
