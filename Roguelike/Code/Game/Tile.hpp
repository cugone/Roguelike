#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Game/Inventory.hpp"

#include <array>
#include <vector>

namespace a2de {
    class Renderer;
    class AnimatedSprite;
}

class TileDefinition;
class Entity;
class Actor;
class Feature;
class Layer;

class Tile {
public:
    Tile();
    Tile(const Tile& other) = default;
    Tile(Tile&& other) = default;
    Tile& operator=(const Tile& other) = default;
    Tile& operator=(Tile&& other) = default;
    ~Tile() = default;

    void Update(a2de::TimeUtils::FPSeconds deltaSeconds);

    void DebugRender(a2de::Renderer& renderer) const;

    void ChangeTypeFromName(const std::string& name);
    void ChangeTypeFromGlyph(char glyph);

    a2de::AABB2 GetBounds() const;
    const TileDefinition* GetDefinition() const;
    TileDefinition* GetDefinition();

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    bool IsOpaque() const;
    bool IsTransparent() const;

    bool IsSolid() const;
    bool IsPassable() const;

    bool HasInventory() const noexcept;
    Item* AddItem(Item* item) noexcept;
    Item* AddItem(const std::string& name) noexcept;

    void SetCoords(int x, int y);
    void SetCoords(const a2de::IntVector2& coords);
    const a2de::IntVector2& GetCoords() const;
    int GetIndexFromCoords() const noexcept;

    std::array<Tile*, 8> GetNeighbors() const;
    Tile* GetNeighbor(const a2de::IntVector3& directionAndLayerOffset) const;
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

    std::vector<Tile*> GetNeighbors(const a2de::IntVector2& direction) const;
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

    void AddVerts() const noexcept;

    a2de::Rgba debugRaycastColor = a2de::Rgba::Red;
    a2de::Rgba highlightColor = a2de::Rgba::White;
    a2de::Rgba color = a2de::Rgba::White;
    Actor* actor{};
    Feature* feature{};
    Layer* layer{};
    std::unique_ptr<Inventory> inventory{};
    uint8_t haveSeen : 1;
    uint8_t canSee : 1;
    uint8_t debug_canSee : 1;
protected:
private:
    void AddVertsForTile() const noexcept;
    void AddVertsForOverlay() const noexcept;
    a2de::AABB2 GetCoordsForOverlay(std::string overlayName) const;
    a2de::AnimatedSprite* GetSpriteForOverlay(std::string overlayName) const;

    TileDefinition* _def{};
    a2de::IntVector2 _tile_coords{};
};
