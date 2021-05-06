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

void Cursor::Update(a2de::TimeUtils::FPSeconds deltaSeconds) {
    _def->GetSprite()->Update(deltaSeconds);
}

a2de::AABB2 Cursor::GetBounds() const {
    return {a2de::Vector2(_tile_coords), a2de::Vector2(_tile_coords + a2de::IntVector2::ONE)};
}

const CursorDefinition* Cursor::GetDefinition() const {
    return _def;
}

CursorDefinition* Cursor::GetDefinition() {
    return _def;
}

void Cursor::SetCoords(int x, int y) {
    SetCoords(a2de::IntVector2{x, y});
}

void Cursor::SetCoords(const a2de::IntVector2& coords) {
    _tile_coords = coords;
}

const a2de::IntVector2& Cursor::GetCoords() const {
    return _tile_coords;
}

void Cursor::AddVertsForCursor(a2de::Mesh::Builder& builder) const {
    const auto vert_left = _tile_coords.x + 0.0f;
    const auto vert_right = _tile_coords.x + 1.0f;
    const auto vert_top = _tile_coords.y + 0.0f;
    const auto vert_bottom = _tile_coords.y + 1.0f;

    const auto vert_bl = a2de::Vector2(vert_left, vert_bottom);
    const auto vert_tl = a2de::Vector2(vert_left, vert_top);
    const auto vert_tr = a2de::Vector2(vert_right, vert_top);
    const auto vert_br = a2de::Vector2(vert_right, vert_bottom);

    const auto& sprite = _def->GetSprite();
    const auto& coords = sprite->GetCurrentTexCoords();

    const auto tx_left = coords.mins.x;
    const auto tx_right = coords.maxs.x;
    const auto tx_top = coords.mins.y;
    const auto tx_bottom = coords.maxs.y;

    const auto tx_bl = a2de::Vector2(tx_left, tx_bottom);
    const auto tx_tl = a2de::Vector2(tx_left, tx_top);
    const auto tx_tr = a2de::Vector2(tx_right, tx_top);
    const auto tx_br = a2de::Vector2(tx_right, tx_bottom);

    builder.Begin(a2de::PrimitiveType::Triangles);
    builder.SetColor(color);
    builder.SetNormal(-a2de::Vector3::Z_AXIS);

    builder.SetUV(tx_bl);
    builder.AddVertex(a2de::Vector3{vert_bl, 0.0f});

    builder.SetUV(tx_tl);
    builder.AddVertex(a2de::Vector3{vert_tl, 0.0f});

    builder.SetUV(tx_tr);
    builder.AddVertex(a2de::Vector3{vert_tr, 0.0f});

    builder.SetUV(tx_br);
    builder.AddVertex(a2de::Vector3{vert_br, 0.0f});

    builder.AddIndicies(a2de::Mesh::Builder::Primitive::Quad);
    builder.End();
}
