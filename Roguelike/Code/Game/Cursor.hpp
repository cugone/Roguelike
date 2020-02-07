#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Vertex3D.hpp"

#include <vector>

class CursorDefinition;
class Entity;
class Layer;
class Renderer;

class Cursor {
public:
    Cursor() = delete;
    Cursor(const Cursor& other) = default;
    Cursor(Cursor&& other) = default;
    Cursor& operator=(const Cursor& other) = default;
    Cursor& operator=(Cursor&& other) = default;
    ~Cursor() = default;

    explicit Cursor(CursorDefinition& def);

    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo) const;

    AABB2 GetBounds() const;
    const CursorDefinition* GetDefinition() const;
    CursorDefinition* GetDefinition();

    void SetCoords(int x, int y);
    void SetCoords(const IntVector2& coords);
    const IntVector2& GetCoords() const;

    Rgba color = Rgba::White;
protected:
private:
    void AddVertsForCursor(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo) const;

    CursorDefinition* _def{};
    IntVector2 _tile_coords{};
};
