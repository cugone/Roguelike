#include "Game/Cursor.hpp"

#include "Engine/Core/BuildConfig.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/CursorDefinition.hpp"
#include "Game/GameCommon.hpp"

Cursor::Cursor(CursorDefinition& def)
    : _def(&def)
{
    /* DO NOTHING */
}

void Cursor::Update(TimeUtils::FPSeconds deltaSeconds) {
    _def->GetSprite()->Update(deltaSeconds);
}

void Cursor::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo) const {
    AddVertsForCursor(verts, ibo);
}

AABB2 Cursor::GetBounds() const {
    return {Vector2(_tile_coords), Vector2(_tile_coords + IntVector2::ONE)};
}

const CursorDefinition* Cursor::GetDefinition() const {
    return _def;
}

CursorDefinition* Cursor::GetDefinition() {
    return _def;
}

void Cursor::SetCoords(int x, int y) {
    SetCoords(IntVector2{x, y});
}

void Cursor::SetCoords(const IntVector2& coords) {
    _tile_coords = coords;
}

const IntVector2& Cursor::GetCoords() const {
    return _tile_coords;
}

void Cursor::AddVertsForCursor(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo) const {
    const auto vert_left = _tile_coords.x + 0.0f;
    const auto vert_right = _tile_coords.x + 1.0f;
    const auto vert_top = _tile_coords.y + 0.0f;
    const auto vert_bottom = _tile_coords.y + 1.0f;

    const auto vert_bl = Vector2(vert_left, vert_bottom);
    const auto vert_tl = Vector2(vert_left, vert_top);
    const auto vert_tr = Vector2(vert_right, vert_top);
    const auto vert_br = Vector2(vert_right, vert_bottom);

    const auto& sprite = _def->GetSprite();
    const auto& coords = sprite->GetCurrentTexCoords();

    const auto tx_left = coords.mins.x;
    const auto tx_right = coords.maxs.x;
    const auto tx_top = coords.mins.y;
    const auto tx_bottom = coords.maxs.y;

    const auto tx_bl = Vector2(tx_left, tx_bottom);
    const auto tx_tl = Vector2(tx_left, tx_top);
    const auto tx_tr = Vector2(tx_right, tx_top);
    const auto tx_br = Vector2(tx_right, tx_bottom);

    verts.push_back(Vertex3D(Vector3(vert_bl, 0.0f), color, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tl, 0.0f), color, tx_tl));
    verts.push_back(Vertex3D(Vector3(vert_tr, 0.0f), color, tx_tr));
    verts.push_back(Vertex3D(Vector3(vert_br, 0.0f), color, tx_br));

    const auto v_s = verts.size();
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 3u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 1u);
}
