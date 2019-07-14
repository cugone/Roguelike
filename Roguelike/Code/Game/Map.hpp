#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/Layer.hpp"
#include "Game/Equipment.hpp"

#include <filesystem>
#include <map>
#include <memory>


class Entity;
struct EntityType;
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
    Tile* GetTile(const IntVector3& locationAndLayerIndex) const;
    Tile* GetTile(int x, int y, int z) const;
    Tile* PickTileFromWorldCoords(const Vector2& worldCoords, int layerIndex) const;
    Tile* PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const;

    bool MoveOrAttack(Actor* actor, Tile* tile);

    void SetDebugGridColor(const Rgba& gridColor);

    mutable Camera2D camera{};
    Actor* player = nullptr;

protected:
private:
    bool LoadFromXML(const XMLElement& elem);
    void LoadNameForMap(const XMLElement& elem);
    void LoadMaterialsForMap(const XMLElement& elem);
    void LoadLayersForMap(const XMLElement& elem);
    void LoadTileDefinitionsForMap(const XMLElement& elem);
    void LoadTileDefinitionsFromFile(const std::filesystem::path& src);
    void LoadEntitiesForMap(const XMLElement& elem);
    void LoadEntitiesFromFile(const std::filesystem::path& src);
    void LoadEntityDefinitionsFromFile(const std::filesystem::path& src);
    void LoadEntityTypesForMap(const XMLElement& elem);
    void LoadEquipmentForMap(const XMLElement& elem);
    void LoadEquipmentFromFile(const std::filesystem::path& src);
    void LoadEquipmentTypesForMap(const XMLElement& elem);
    void LoadEquipmentDefinitionsFromFile(const std::filesystem::path& src);
    void PlaceEntitiesOnMap(const XMLElement& elem);
    std::vector<EntityType*> GetEntityTypesByName(const std::string& name);
    std::vector<EquipmentType*> GetEquipmentTypesByName(const std::string& name);

    void UpdateEntities(TimeUtils::FPSeconds deltaSeconds);
    void UpdateEntityAI(TimeUtils::FPSeconds deltaSeconds);

    void BringLayerToFront(std::size_t i);

    std::string _name{};
    std::vector<std::unique_ptr<Layer>> _layers{};
    Renderer& _renderer;
    Material* _default_tileMaterial{};
    Material* _current_tileMaterial{};
    std::vector<Entity*> _entities{};
    std::vector<Actor*> _actors{};
    std::multimap<std::string, std::unique_ptr<EntityType>> _entity_types{};
    std::vector<std::unique_ptr<Equipment>> _equipments{};
    std::multimap<std::string, std::unique_ptr<EquipmentType>> _equipment_types{};

    std::shared_ptr<SpriteSheet> _tileset_sheet{};
    std::shared_ptr<SpriteSheet> _entity_sheet{};
    std::shared_ptr<SpriteSheet> _equipment_sheet{};
    float _camera_speed = 1.0f;
    static unsigned long long default_map_index;
};
