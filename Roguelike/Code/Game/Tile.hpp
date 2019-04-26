#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"
#include <vector>

class Renderer;
class TileDefinition;

class Tile {
public:
    Tile();
    Tile(const Tile& other) = default;
    Tile(Tile&& other) = default;
    Tile& operator=(const Tile& other) = default;
    Tile& operator=(Tile&& other) = default;
    ~Tile() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(Renderer& renderer, std::vector<Vertex3D>& verts, size_t layer_index) const;

    void ChangeTypeFromName(const std::string& name);

    AABB2 GetBounds() const;
    const TileDefinition* GetDefinition() const;

    bool IsOpaque() const;
    bool IsTransparent() const;

    bool IsSolid() const;
    bool IsPassable() const;

protected:
private:
    TileDefinition* _def{};
    IntVector2 _tile_coords{};
};
