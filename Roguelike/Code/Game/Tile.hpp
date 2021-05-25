#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Game/Inventory.hpp"

#include <array>
#include <vector>

class Renderer;
class AnimatedSprite;
class TileDefinition;
class Entity;
class Actor;
class Feature;
class Map;
class Layer;

class Tile {
public:
    Tile() = default;
    Tile(const Tile& other) = default;
    Tile(Tile&& other) = default;
    Tile& operator=(const Tile& other) = default;
    Tile& operator=(Tile&& other) = default;
    ~Tile() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds);

    void DebugRender(Renderer& renderer) const;

    void ChangeTypeFromName(const std::string& name);
    void ChangeTypeFromGlyph(char glyph);

    AABB2 GetBounds() const;

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    bool IsLightDirty() const;
    bool IsOpaque() const;
    bool IsTransparent() const;

    bool IsSolid() const;
    bool IsPassable() const;

    bool IsEntrance() const;
    bool IsExit() const;

    void SetEntrance() noexcept;
    void SetExit() noexcept;
    void ClearEntrance() noexcept;
    void ClearExit() noexcept;

    bool HasInventory() const noexcept;
    Item* AddItem(Item* item) noexcept;
    Item* AddItem(const std::string& name) noexcept;

    const IntVector2 GetCoords() const;
    void SetCoords(int x, int y);
    void SetCoords(const IntVector2& coords);
    
    std::size_t GetIndexFromCoords() const noexcept;

    uint32_t GetFlags() const noexcept;
    void SetFlags(uint32_t flags) noexcept;

    void CalculateLightValue() noexcept;
    uint32_t GetLightValue() const noexcept;
    void SetLightValue(uint32_t newValue) noexcept;
    void IncrementLightValue(int value = 1) noexcept;
    void DecrementLightValue(int value = 1) noexcept;

    void ClearLightDirty() noexcept;
    void SetLightDirty() noexcept;
    void DirtyLight() noexcept;

    void ClearOpaque() noexcept;
    void SetOpaque() noexcept;

    void ClearSolid() noexcept;
    void SetSolid() noexcept;

    std::array<Tile*, 8> GetNeighbors() const;
    std::array<Tile*, 4> GetCardinalNeighbors() const;
    std::array<Tile*, 4> GetOrdinalNeighbors() const;
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

    uint32_t GetMaxLightValueFromNeighbors() const noexcept;

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
    const std::string GetType() const noexcept;

    void AddVerts() const noexcept;

    Rgba debugRaycastColor = Rgba::Red;
    Rgba highlightColor = Rgba::White;
    Rgba color = Rgba::White;
    Actor* actor{};
    Feature* feature{};
    Layer* layer{};
    std::unique_ptr<Inventory> inventory{};
    uint8_t haveSeen : 1;
    uint8_t canSee : 1;
protected:
private:
    void AddVertsForTile() const noexcept;
    void AddVertsForOverlay() const noexcept;
    AABB2 GetCoordsForOverlay(std::string overlayName) const;
    AnimatedSprite* GetSpriteForOverlay(std::string overlayName) const;

    std::string _type{"void"};
    uint32_t _flags_coords_lightvalue{};
};

class TileInfo {
public:
    Layer* layer{};
    std::size_t index{};

    bool IsSky() const noexcept;
    bool IsAtEdge() const noexcept;

    bool IsLightDirty() const noexcept;
    void ClearLightDirty() noexcept;
    void SetLightDirty() noexcept;

    bool IsOpaque() const noexcept;
    void ClearOpaque() noexcept;
    void SetOpaque() noexcept;

    bool IsSolid() const noexcept;
    void ClearSolid() noexcept;
    void SetSolid() noexcept;

    bool MoveEast() noexcept;
    bool MoveWest() noexcept;
    bool MoveNorth() noexcept;
    bool MoveSouth() noexcept;

    TileInfo GetNorthNeighbor() const noexcept;
    TileInfo GetSouthNeighbor() const noexcept;
    TileInfo GetEastNeighbor() const noexcept;
    TileInfo GetWestNeighbor() const noexcept;

    uint32_t GetLightValue() const noexcept;
    void SetLightValue(uint32_t newValue) noexcept;
    uint32_t GetSelfIlluminationValue() const noexcept;
    uint32_t GetMaxLightValueFromNeighbors() const noexcept;

protected:
private:

};
