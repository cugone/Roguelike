#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"
#include <vector>

class TileDefinition;
class Entity;
class Layer;

class Tile {
public:
    Tile();
    Tile(const Tile& other) = default;
    Tile(Tile&& other) = default;
    Tile& operator=(const Tile& other) = default;
    Tile& operator=(Tile&& other) = default;
    ~Tile() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;

    void ChangeTypeFromName(const std::string& name);
    void ChangeTypeFromGlyph(char glyph);

    AABB2 GetBounds() const;
    const TileDefinition* GetDefinition() const;
    TileDefinition* GetDefinition();

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    bool IsOpaque() const;
    bool IsTransparent() const;

    bool IsSolid() const;
    bool IsPassable() const;

    void SetCoords(int x, int y);
    void SetCoords(const IntVector2& coords);
    const IntVector2& GetCoords() const;

    Rgba color = Rgba::White;
    Entity* entity{};
    Layer* layer = nullptr;
protected:
private:
    void AddVertsForTile(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;

    TileDefinition* _def{};
    IntVector2 _tile_coords{};
};
