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

void Tile::Render(std::vector<Vertex3D>& verts, const Rgba& layer_color, size_t layer_index) const {
    if(IsInvisible()) {
        return;
    }

    const auto& coords = _def->GetSheet()->GetTexCoordsFromSpriteCoords(_def->index);

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
    verts.push_back(Vertex3D(Vector3(vert_bl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tl));
    verts.push_back(Vertex3D(Vector3(vert_tr, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tr));

    verts.push_back(Vertex3D(Vector3(vert_bl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tr, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tr));
    verts.push_back(Vertex3D(Vector3(vert_br, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_br));
}

void Tile::ChangeTypeFromName(const std::string& name) {
    if(_def && _def->name == name) {
        return;
    }
    if(auto new_def = TileDefinition::GetTileDefinitionByName(name)) {
        _def = new_def;
    }
}

void Tile::ChangeTypeFromGlyph(char glyph) {
    if(_def && _def->glyph == glyph) {
        return;
    }
    if(auto new_def = TileDefinition::GetTileDefinitionByGlyph(glyph)) {
        _def = new_def;
    }
}

AABB2 Tile::GetBounds() const {
    return { Vector2(_tile_coords), Vector2(_tile_coords + IntVector2::ONE) };
}

TileDefinition* Tile::GetDefinition() {
    return _def;
}

bool Tile::IsVisible() const {
    return _def->is_visible;
}

bool Tile::IsNotVisible() const {
    return !IsVisible();
}

bool Tile::IsInvisible() const {
    return IsNotVisible();
}

bool Tile::IsOpaque() const {
    return _def->is_opaque || color.a == 0;
}

bool Tile::IsTransparent() const {
    return !IsOpaque();
}

bool Tile::IsSolid() const {
    return _def->is_solid;
}

bool Tile::IsPassable() const {
    return !IsSolid();
}

void Tile::SetCoords(int x, int y) {
    SetCoords(IntVector2{ x, y });
}

void Tile::SetCoords(const IntVector2& coords) {
    _tile_coords = coords;
}
