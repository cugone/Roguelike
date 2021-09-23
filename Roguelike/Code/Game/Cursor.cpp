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

AABB2 Cursor::GetBounds() const {
    return {Vector2(_tile_coords), Vector2(_tile_coords + IntVector2::One)};
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
