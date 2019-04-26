#include "Game/Tile.hpp"

#include "Engine/Core/BuildConfig.hpp"

Tile::Tile()
    : _def(TileDefinition::GetTileDefinitionByName("void"))
{

}

void Tile::Update(TimeUtils::FPSeconds deltaSeconds) {
    UNUSED(deltaSeconds);
}

void Tile::Render(Renderer& renderer, std::vector<Vertex3D>& verts, size_t layer_index) const {

}

void Tile::ChangeTypeFromName(const std::string& name) {
    if(_def->_name == name) {
        return;
    }
    auto new_def = TileDefinition::GetTileDefinitionByName(name);
    _def = new_def;
}

AABB2 Tile::GetBounds() const {
    return { Vector2(_tileCoords), Vector2(_tileCoords + IntVector2::ONE) };
}

const TileDefinition* Tile::GetDefinition() const {
    return _def;
}

bool Tile::IsOpaque() const {
    return _def->_is_opaque;
}

bool Tile::IsTransparent() const {
    return !IsOpaque();
}

bool Tile::IsSolid() const {
    return _def->_is_solid;
}

bool Tile::IsPassable() const {
    return !IsSolid();
}
