#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/EntityDefinition.hpp"
#include "Game/EntityText.hpp"
#include "Game/Inventory.hpp"
#include "Game/Layer.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <vector>

namespace a2de {
    class Material;
    class Renderer;
    class SpriteSheet;
}

class Entity;
class Actor;
class TileDefinition;
class MapGenerator;
class Pathfinder;

class Map {
public:

    struct RaycastResult2D {
        bool didImpact{false};
        a2de::Vector2 impactPosition{};
        std::set<a2de::IntVector2> impactTileCoords{};
        float impactFraction{1.0f};
        a2de::Vector2 impactSurfaceNormal{};
    };


    Map() noexcept = default;
    explicit Map(a2de::Renderer& renderer, const a2de::XMLElement& elem) noexcept;
    Map(const Map& other) = default;
    Map(Map&& other) = default;
    Map& operator=(const Map& other) = default;
    Map& operator=(Map&& other) = default;
    ~Map() noexcept;

    void BeginFrame();
    void Update(a2de::TimeUtils::FPSeconds deltaSeconds);
    void UpdateLayers(a2de::TimeUtils::FPSeconds deltaSeconds);
    void SetPriorityLayer(std::size_t i);
    void Render(a2de::Renderer& renderer) const;
    void DebugRender(a2de::Renderer& renderer) const;
    void EndFrame();

    bool IsTileInArea(const a2de::AABB2& bounds, const a2de::IntVector2& tileCoords) const;
    bool IsTileInArea(const a2de::AABB2& bounds, const a2de::IntVector3& tileCoords) const;
    bool IsTileInArea(const a2de::AABB2& bounds, const Tile* tile) const;
    bool IsTileInView(const a2de::IntVector2& tileCoords) const;
    bool IsTileInView(const a2de::IntVector3& tileCoords) const;
    bool IsTileInView(const Tile* tile) const;
    bool IsEntityInView(Entity* entity) const;

    bool IsTileOpaque(const a2de::IntVector2& tileCoords) const;
    bool IsTileOpaque(const a2de::IntVector3& tileCoords) const;
    bool IsTileOpaque(Tile* tile) const;

    bool IsTileSolid(const a2de::IntVector2& tileCoords) const;
    bool IsTileSolid(const a2de::IntVector3& tileCoords) const;
    bool IsTileSolid(Tile* tile) const;

    bool IsTileOpaqueOrSolid(const a2de::IntVector2& tileCoords) const;
    bool IsTileOpaqueOrSolid(const a2de::IntVector3& tileCoords) const;
    bool IsTileOpaqueOrSolid(Tile* tile) const;

    bool IsTileVisible(const a2de::IntVector2& tileCoords) const;
    bool IsTileVisible(const a2de::IntVector3& tileCoords) const;
    bool IsTileVisible(const Tile* tile) const;

    bool IsTilePassable(const a2de::IntVector2& tileCoords) const;
    bool IsTilePassable(const a2de::IntVector3& tileCoords) const;
    bool IsTilePassable(const Tile* tile) const;

    void ZoomOut() noexcept;
    void ZoomIn() noexcept;

    void FocusTileAt(const a2de::IntVector3& position);
    void FocusEntity(const Entity* entity);

    RaycastResult2D HasLineOfSight(const a2de::Vector2& startPosition, const a2de::Vector2& endPosition) const;
    RaycastResult2D HasLineOfSight(const a2de::Vector2& startPosition, const a2de::Vector2& direction, float maxDistance) const;
    bool IsTileWithinDistance(const Tile& startTile, unsigned int manhattanDist) const;

    bool IsTileWithinDistance(const Tile& startTile, float dist) const;

    std::vector<Tile*> GetTilesInArea(const a2de::AABB2& bounds) const;
    std::vector<Tile*> GetTilesWithinDistance(const Tile& startTile, unsigned int manhattanDist) const;
    std::vector<Tile*> GetTilesWithinDistance(const Tile& startTile, float dist) const;
    std::vector<Tile*> GetVisibleTilesWithinDistance(const Tile& startTile, float dist) const;
    std::vector<Tile*> GetVisibleTilesWithinDistance(const Tile& startTile, unsigned int manhattanDist) const;

    template<typename Pr>
    std::vector<Tile*> GetTilesWithinDistance(const Tile& startTile, float distance, Pr&& predicate) const {
        auto* layer0 = GetLayer(0);
        if(!layer0) {
            return {};
        }
        const auto& start_coords = startTile.GetCoords();
        std::vector<Tile*> results;
        for(auto& tile : *layer0) {
            const auto& end_coords = tile.GetCoords();
            if(predicate(start_coords, end_coords) < distance) {
                results.push_back(&tile);
            }
        }
        return results;
    }

    template<typename Pr>
    std::vector<Tile*> GetTilesAtDistance(const Tile& startTile, float distance, Pr&& predicate) const {
        auto* layer0 = GetLayer(0);
        if(!layer0) {
            return {};
        }
        const auto& start_coords = startTile.GetCoords();
        std::vector<Tile*> results;
        for(auto& tile : *layer0) {
            const auto& end_coords = tile.GetCoords();
            const bool pLessD = predicate(start_coords, end_coords) < distance;
            const bool dLessP = distance < predicate(start_coords, end_coords);
            if(!pLessD && !dLessP) {
                results.push_back(&tile);
            }
        }
        return results;
    }


    RaycastResult2D StepAndSample(const a2de::Vector2& startPosition, const a2de::Vector2& endPosition, float sampleRate) const;
    RaycastResult2D StepAndSample(const a2de::Vector2& startPosition, const a2de::Vector2& direction, float maxDistance, float sampleRate) const;


    //************************************
    // Method:    Raycast
    // FullName:  Map::Raycast
    // Access:    public 
    // Returns:   Map::RaycastResult2D
    // Qualifier: const
    // Parameter: const Vector2& startPosition: The start position in world units.
    // Parameter: const Vector2& direction: The direction of the ray.
    // Parameter: float maxDistance: The maximum distance in world units.
    // Parameter: Pr predicate: A predicate function that takes an IntVector2 as an argument, representing the tile coordinate of the current tile, and returns a bool.
    //     The predicate shall return true on impact.
    //************************************
    template<typename Pr>
    RaycastResult2D Raycast(const a2de::Vector2& startPosition, const a2de::Vector2& direction, float maxDistance, bool ignoreSelf, Pr predicate) const {
        const auto endPosition = startPosition + (direction * maxDistance);
        a2de::IntVector2 currentTileCoords{startPosition};
        a2de::IntVector2 endTileCoords{endPosition};

        const auto D = endPosition - startPosition;

        float tDeltaX = (std::numeric_limits<float>::max)();
        if(!a2de::MathUtils::IsEquivalent(D.x, 0.0f)) {
            tDeltaX = 1.0f / std::abs(D.x);
        }
        int tileStepX = 0;
        if(D.x > 0) {
            tileStepX = 1;
        }
        if(D.x < 0) {
            tileStepX = -1;
        }
        int offsetToLeadingEdgeX = (tileStepX + 1) / 2;
        float firstVerticalIntersectionX = static_cast<float>(currentTileCoords.x + offsetToLeadingEdgeX);
        float tOfNextXCrossing = std::abs(firstVerticalIntersectionX - startPosition.x) * tDeltaX;

        float tDeltaY = (std::numeric_limits<float>::max)();
        if(!a2de::MathUtils::IsEquivalent(D.y, 0.0f)) {
            tDeltaY = 1.0f / std::abs(D.y);
        }
        int tileStepY = 0;
        if(D.y > 0) {
            tileStepY = 1;
        }
        if(D.y < 0) {
            tileStepY = -1;
        }
        int offsetToLeadingEdgeY = (tileStepY + 1) / 2;
        float firstVerticalIntersectionY = static_cast<float>(currentTileCoords.y + offsetToLeadingEdgeY);
        float tOfNextYCrossing = std::abs(firstVerticalIntersectionY - startPosition.y) * tDeltaY;

        Map::RaycastResult2D result;
        if(!ignoreSelf && predicate(currentTileCoords)) {
            result.didImpact = true;
            result.impactFraction = 0.0f;
            result.impactPosition = startPosition;
            result.impactTileCoords.insert(currentTileCoords);
            result.impactSurfaceNormal = -direction;
            return result;
        }

        while(true) {
            result.impactTileCoords.insert(currentTileCoords);
            if(tOfNextXCrossing < tOfNextYCrossing) {
                if(tOfNextXCrossing > 1.0f) {
                    result.didImpact = false;
                    return result;
                }
                currentTileCoords.x += tileStepX;
                if(predicate(currentTileCoords)) {
                    result.didImpact = true;
                    result.impactFraction = tOfNextXCrossing;
                    result.impactPosition = startPosition + (D * result.impactFraction);
                    result.impactTileCoords.insert(currentTileCoords);
                    result.impactSurfaceNormal = a2de::Vector2(static_cast<float>(-tileStepX), 0.0f);
                    return result;
                }
                tOfNextXCrossing += tDeltaX;
            } else {
                if(tOfNextYCrossing > 1.0f) {
                    result.didImpact = false;
                    return result;
                }
                currentTileCoords.y += tileStepY;
                if(predicate(currentTileCoords)) {
                    result.didImpact = true;
                    result.impactFraction = tOfNextYCrossing;
                    result.impactPosition = startPosition + (D * result.impactFraction);
                    result.impactTileCoords.insert(currentTileCoords);
                    result.impactSurfaceNormal = a2de::Vector2(0.0f, static_cast<float>(-tileStepY));
                    return result;
                }
                tOfNextYCrossing += tDeltaY;
            }
        }
        return result;
    }

    //************************************
    // Method:    Raycast
    // FullName:  Map::Raycast
    // Access:    public 
    // Returns:   Map::RaycastResult2D
    // Qualifier: const
    // Parameter: const Vector2& startPosition: The start position in world units.
    // Parameter: const Vector2& endPosition: The end position in world units.
    // Parameter: bool ignoreSelf: Ignore the initial tile.
    // Parameter: Pr predicate: A predicate function that takes an IntVector2 as an argument, representing the tile coordinate of the current tile, and returns a bool.
    //     The predicate shall return true on impact.
    //************************************
    template<typename Pr>
    RaycastResult2D Raycast(const a2de::Vector2& startPosition, const a2de::Vector2& endPosition, bool ignoreSelf, Pr predicate) const {
        const auto displacement = endPosition - startPosition;
        const auto direction = displacement.GetNormalize();
        float length = displacement.CalcLength();
        return Raycast(startPosition, direction, length, ignoreSelf, predicate);
    }

    a2de::AABB2 CalcWorldBounds() const;
    a2de::AABB2 CalcCameraBounds() const;
    a2de::Vector2 CalcMaxDimensions() const;
    a2de::Material* GetTileMaterial() const;
    void SetTileMaterial(a2de::Material* material);
    void ResetTileMaterial();
    std::size_t GetLayerCount() const;
    Layer* GetLayer(std::size_t index) const;
    std::vector<Tile*> GetTiles(const a2de::IntVector2& location) const;
    std::vector<Tile*> GetTiles(int x, int y) const;
    std::vector<Tile*> PickTilesFromWorldCoords(const a2de::Vector2& worldCoords) const;
    std::vector<Tile*> PickTilesFromMouseCoords(const a2de::Vector2& mouseCoords) const;
    a2de::Vector2 WorldCoordsToScreenCoords(const a2de::Vector2& worldCoords) const;
    a2de::Vector2 ScreenCoordsToWorldCoords(const a2de::Vector2& screenCoords) const;
    a2de::IntVector2 TileCoordsFromWorldCoords(const a2de::Vector2& worldCoords) const;

    Tile* GetTile(const a2de::IntVector3& locationAndLayerIndex) const;
    Tile* GetTile(int x, int y, int z) const;
    Tile* PickTileFromWorldCoords(const a2de::Vector2& worldCoords, int layerIndex) const;
    Tile* PickTileFromMouseCoords(const a2de::Vector2& mouseCoords, int layerIndex) const;

    bool MoveOrAttack(Actor* actor, Tile* tile);

    void SetDebugGridColor(const a2de::Rgba& gridColor);

    a2de::OrthographicCameraController cameraController{};
    Actor* player = nullptr;

    void KillEntity(Entity& e);
    void KillActor(Actor& a);
    void KillFeature(Feature& f);
    const std::vector<Entity*>& GetEntities() const noexcept;
    const std::vector<EntityText*>& GetTextEntities() const noexcept;

    static inline constexpr std::size_t max_layers = 9u;

    void CreateTextEntity(const TextEntityDesc& desc) noexcept;
    void CreateTextEntityAt(const a2de::IntVector2& tileCoords, TextEntityDesc desc) noexcept;

    template<typename F>
    void ShakeCamera(F&& f) noexcept {
        cameraController.GetCamera().trauma = f();
    }

    std::size_t DebugTilesInViewCount() const;
    std::size_t DebugVisibleTilesInViewCount() const;

    void GenerateMap(const a2de::XMLElement& elem) noexcept;
    void RegenerateMap() noexcept;

    Pathfinder* GetPathfinder() const noexcept;

    std::unique_ptr<MapGenerator> _map_generator{};

protected:
private:
    bool LoadFromXML(const a2de::XMLElement& elem);
    void LoadNameForMap(const a2de::XMLElement& elem);
    void LoadMaterialsForMap(const a2de::XMLElement& elem);
    void LoadGenerator(const a2de::XMLElement& elem);
    void CreateGeneratorFromTypename(const a2de::XMLElement& elem);
    void LoadTileDefinitionsForMap(const a2de::XMLElement& elem);
    void LoadTileDefinitionsFromFile(const std::filesystem::path& src);
    void LoadActorsForMap(const a2de::XMLElement& elem);
    void LoadFeaturesForMap(const a2de::XMLElement& elem);
    void LoadItemsForMap(const a2de::XMLElement& elem);

    void UpdateTextEntities(a2de::TimeUtils::FPSeconds deltaSeconds);
    void UpdateActorAI(a2de::TimeUtils::FPSeconds deltaSeconds);
    void UpdateEntities(a2de::TimeUtils::FPSeconds deltaSeconds);

    void BringLayerToFront(std::size_t i);

    void ShakeCamera(const a2de::IntVector2& from, const a2de::IntVector2& to) noexcept;

    std::string _name{};
    std::vector<std::unique_ptr<Layer>> _layers{};
    a2de::Renderer& _renderer;
    const a2de::XMLElement& _root_xml_element;
    a2de::Material* _default_tileMaterial{};
    a2de::Material* _current_tileMaterial{};
    std::unique_ptr<Pathfinder> _pathfinder{};
    std::vector<Entity*> _entities{};
    std::vector<EntityText*> _text_entities{};
    std::vector<Actor*> _actors{};
    std::vector<Feature*> _features{};
    std::shared_ptr<a2de::SpriteSheet> _tileset_sheet{};

    float _camera_speed = 1.0f;
    mutable std::size_t _debug_tiles_in_view_count{};
    mutable std::size_t _debug_visible_tiles_in_view_count{};
    static inline unsigned long long default_map_index = 0ull;

    friend class MapGenerator;
    friend class HeightMapGenerator;
    friend class FileMapGenerator;
    friend class XmlMapGenerator;
    friend class RoomsMapGenerator;
    friend class RoomsAndCorridorsMapGenerator;
public:
    std::vector<Tile*> GetViewableTiles() const noexcept;
};
