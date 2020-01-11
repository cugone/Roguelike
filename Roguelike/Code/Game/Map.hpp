#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/EntityDefinition.hpp"
#include "Game/EntityText.hpp"
#include "Game/Inventory.hpp"
#include "Game/Layer.hpp"

#include <filesystem>
#include <map>
#include <memory>

class Entity;
class Actor;
class Material;
class Renderer;
class TileDefinition;
class SpriteSheet;

class Map {
public:
    Map() noexcept = default;
    explicit Map(Renderer& renderer, const XMLElement& elem) noexcept;
    Map(const Map& other) = default;
    Map(Map&& other) = default;
    Map& operator=(const Map& other) = default;
    Map& operator=(Map&& other) = default;
    ~Map() noexcept;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void UpdateLayers(TimeUtils::FPSeconds deltaSeconds);
    void SetPriorityLayer(std::size_t i);
    void Render(Renderer& renderer) const;
    void DebugRender(Renderer& renderer) const;
    void EndFrame();

    bool IsTileInView(const IntVector2& tileCoords);
    bool IsTileInView(const IntVector3& tileCoords);
    bool IsTileInView(Tile* tile);
    bool IsEntityInView(Entity* entity);

    void FocusTileAt(const IntVector3& position);
    void FocusEntity(Entity* entity);

    AABB2 CalcWorldBounds() const;
    Vector2 CalcMaxDimensions() const;
    float CalcMaxViewHeight() const;
    Material* GetTileMaterial() const;
    void SetTileMaterial(Material* material);
    void ResetTileMaterial();
    std::size_t GetLayerCount() const;
    Layer* GetLayer(std::size_t index) const;
    std::vector<Tile*> GetTiles(const IntVector2& location) const;
    std::vector<Tile*> GetTiles(int x, int y) const;
    std::vector<Tile*> PickTilesFromWorldCoords(const Vector2& worldCoords) const;
    std::vector<Tile*> PickTilesFromMouseCoords(const Vector2& mouseCoords) const;
    Vector2 WorldCoordsToScreenCoords(const Vector2& worldCoords) const;
    Vector2 ScreenCoordsToWorldCoords(const Vector2& screenCoords) const;

    Tile* GetTile(const IntVector3& locationAndLayerIndex) const;
    Tile* GetTile(int x, int y, int z) const;
    Tile* PickTileFromWorldCoords(const Vector2& worldCoords, int layerIndex) const;
    Tile* PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const;

    bool MoveOrAttack(Actor* actor, Tile* tile);

    void SetDebugGridColor(const Rgba& gridColor);

    mutable Camera2D camera{};
    Actor* player = nullptr;

    void KillEntity(Entity& e);
    std::vector<Entity*> GetEntities() const noexcept;

    static inline constexpr std::size_t max_layers = 9u;
protected:
private:
    bool LoadFromXML(const XMLElement& elem);
    void LoadNameForMap(const XMLElement& elem);
    void LoadMaterialsForMap(const XMLElement& elem);
    void LoadLayersForMap(const XMLElement& elem);
    void LoadTileDefinitionsForMap(const XMLElement& elem);
    void LoadTileDefinitionsFromFile(const std::filesystem::path& src);
    void LoadActorsForMap(const XMLElement& elem);
    void LoadFeaturesForMap(const XMLElement& elem);
    void LoadItemsForMap(const XMLElement& elem);

    void UpdateEntities(TimeUtils::FPSeconds deltaSeconds);
    void UpdateEntityAI(TimeUtils::FPSeconds deltaSeconds);

    void BringLayerToFront(std::size_t i);
    void CreateTextEntity(const TextEntityDesc& desc) noexcept;
    void CreateTextEntityAt(const IntVector2& tileCoords, const TextEntityDesc& desc) noexcept;

    void ShakeCamera(const IntVector2& from, const IntVector2& to) noexcept;

    std::string _name{};
    std::vector<std::unique_ptr<Layer>> _layers{};
    Renderer& _renderer;
    Material* _default_tileMaterial{};
    Material* _current_tileMaterial{};
    std::vector<Entity*> _entities{};
    std::shared_ptr<SpriteSheet> _tileset_sheet{};
    float _camera_speed = 1.0f;
    static inline unsigned long long default_map_index = 0ull;
};
