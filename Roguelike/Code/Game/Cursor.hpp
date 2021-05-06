#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Mesh.hpp"

#include <vector>

namespace a2de {
    class Renderer;
}

class CursorDefinition;
class Entity;
class Layer;

class Cursor {
public:
    Cursor() = delete;
    Cursor(const Cursor& other) = default;
    Cursor(Cursor&& other) = default;
    Cursor& operator=(const Cursor& other) = default;
    Cursor& operator=(Cursor&& other) = default;
    ~Cursor() = default;

    explicit Cursor(CursorDefinition& def);

    void Update(a2de::TimeUtils::FPSeconds deltaSeconds);

    a2de::AABB2 GetBounds() const;
    const CursorDefinition* GetDefinition() const;
    CursorDefinition* GetDefinition();

    void SetCoords(int x, int y);
    void SetCoords(const a2de::IntVector2& coords);
    const a2de::IntVector2& GetCoords() const;

    a2de::Rgba color = a2de::Rgba::White;

    void AddVertsForCursor(a2de::Mesh::Builder& builder) const;

protected:
private:

    CursorDefinition* _def{};
    a2de::IntVector2 _tile_coords{};
};
