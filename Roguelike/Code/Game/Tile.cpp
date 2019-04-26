#include "Game/Tile.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/TileDefinition.hpp"

Tile::Tile()
    : _def(TileDefinition::GetTileDefinitionByName("void"))
{
    /* DO NOTHING */
}

void Tile::Update(TimeUtils::FPSeconds deltaSeconds) {
    UNUSED(deltaSeconds);
}

void Tile::Render(std::vector<Vertex3D>& verts, size_t layer_index) const {

    if(IsTransparent()) {
        return;
    }

    const auto& coords = _def->GetSheet()->GetTexCoordsFromSpriteCoords(_def->_index);

    auto vert_left = _tile_coords.x + 0.0f;
    auto vert_right = _tile_coords.x + 1.0f;
    auto vert_top = _tile_coords.y + 0.0f;
    auto vert_bottom = _tile_coords.y + 1.0f;

    auto vert_bl = Vector2(vert_left, vert_bottom);
    auto vert_tl = Vector2(vert_left, vert_top);
    auto vert_tr = Vector2(vert_right, vert_top);
    auto vert_br = Vector2(vert_right, vert_bottom);

    auto tx_left = coords.mins.x;
    auto tx_right = coords.maxs.x;
    auto tx_top = coords.mins.y;
    auto tx_bottom = coords.maxs.y;

    auto tx_bl = Vector2(tx_left, tx_bottom);
    auto tx_tl = Vector2(tx_left, tx_top);
    auto tx_tr = Vector2(tx_right, tx_top);
    auto tx_br = Vector2(tx_right, tx_bottom);

    float z = static_cast<float>(layer_index);
    verts.push_back(Vertex3D(Vector3(vert_bl, z), Rgba::White, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tl, z), Rgba::White, tx_tl));
    verts.push_back(Vertex3D(Vector3(vert_tr, z), Rgba::White, tx_tr));

    verts.push_back(Vertex3D(Vector3(vert_bl, z), Rgba::White, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tr, z), Rgba::White, tx_tr));
    verts.push_back(Vertex3D(Vector3(vert_br, z), Rgba::White, tx_br));
}

void Tile::ChangeTypeFromName(const std::string& name) {
    if(_def->_name == name) {
        return;
    }
    auto new_def = TileDefinition::GetTileDefinitionByName(name);
    _def = new_def;
}

AABB2 Tile::GetBounds() const {
    return { Vector2(_tile_coords), Vector2(_tile_coords + IntVector2::ONE) };
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
