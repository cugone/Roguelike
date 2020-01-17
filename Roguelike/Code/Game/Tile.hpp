#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"
#include <vector>

class TileDefinition;
class Entity;
class Actor;
class Feature;
class Layer;
class Renderer;

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
    void DebugRender(Renderer& renderer) const;

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

    Tile* GetNeighbor(const IntVector3& directionAndLayerOffset) const;
    Tile* GetNorthNeighbor() const;
    Tile* GetNorthEastNeighbor() const;
    Tile* GetEastNeighbor() const;
    Tile* GetSouthEastNeighbor() const;
    Tile* GetSouthNeighbor() const;
    Tile* GetSouthWestNeighbor() const;
    Tile* GetWestNeighbor() const;
    Tile* GetNorthWestNeighbor() const;
    Tile* GetUpNeighbor() const;
    Tile* GetDownNeighbor() const;

    std::vector<Tile*> GetNeighbors(const IntVector2& direction) const;
    std::vector<Tile*> GetNorthNeighbors() const;
    std::vector<Tile*> GetNorthEastNeighbors() const;
    std::vector<Tile*> GetEastNeighbors() const;
    std::vector<Tile*> GetSouthEastNeighbors() const;
    std::vector<Tile*> GetSouthNeighbors() const;
    std::vector<Tile*> GetSouthWestNeighbors() const;
    std::vector<Tile*> GetWestNeighbors() const;
    std::vector<Tile*> GetNorthWestNeighbors() const;

    Entity* GetEntity() const noexcept;
    void SetEntity(Entity* e) noexcept;

    Rgba color = Rgba::White;
    Actor* actor{};
    Feature* feature{};
    Layer* layer = nullptr;
    bool haveSeen = false;
    bool canSee = false;
protected:
private:
    void AddVertsForTile(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;
    void AddVertsForOverlay(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;
    AABB2 GetCoordsForOverlay(std::string overlayName) const;

    TileDefinition* _def{};
    IntVector2 _tile_coords{};
};
