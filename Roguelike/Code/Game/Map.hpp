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
#include <set>
#include <stack>
#include <vector>

class Entity;
class Actor;
class Material;
class Renderer;
class TileDefinition;
class SpriteSheet;
class MapGenerator;

class Map {
public:

    struct RaycastResult2D {
        bool didImpact{false};
        Vector2 impactPosition{};
        std::set<IntVector2> impactTileCoords{};
        float impactFraction{1.0f};
        Vector2 impactSurfaceNormal{};
    };


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

    bool IsTileInView(const IntVector2& tileCoords) const;
    bool IsTileInView(const IntVector3& tileCoords) const;
    bool IsTileInView(const Tile* tile) const;
    bool IsEntityInView(Entity* entity) const;

    bool IsTileOpaque(const IntVector2& tileCoords) const;
    bool IsTileOpaque(const IntVector3& tileCoords) const;
    bool IsTileOpaque(Tile* tile) const;

    bool IsTileSolid(const IntVector2& tileCoords) const;
    bool IsTileSolid(const IntVector3& tileCoords) const;
    bool IsTileSolid(Tile* tile) const;

    bool IsTileOpaqueOrSolid(const IntVector2& tileCoords) const;
    bool IsTileOpaqueOrSolid(const IntVector3& tileCoords) const;
    bool IsTileOpaqueOrSolid(Tile* tile) const;

    bool IsTileVisible(const IntVector2& tileCoords) const;
    bool IsTileVisible(const IntVector3& tileCoords) const;
    bool IsTileVisible(const Tile* tile) const;

    bool IsTilePassable(const IntVector2& tileCoords) const;
    bool IsTilePassable(const IntVector3& tileCoords) const;
    bool IsTilePassable(const Tile* tile) const;

    void FocusTileAt(const IntVector3& position);
    void FocusEntity(const Entity* entity);

    RaycastResult2D HasLineOfSight(const Vector2& startPosition, const Vector2& endPosition) const;
    RaycastResult2D HasLineOfSight(const Vector2& startPosition, const Vector2& direction, float maxDistance) const;
    bool IsTileWithinDistance(const Tile& startTile, unsigned int manhattanDist) const;

    bool IsTileWithinDistance(const Tile& startTile, float dist) const;

    std::vector<Tile*> GetTilesInArea(const AABB2& bounds) const;
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
        const auto start_coords = startTile.GetCoords();
        std::vector<Tile*> results;
        for(auto& tile : *layer0) {
            const auto end_coords = tile.GetCoords();
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
        const auto start_coords = startTile.GetCoords();
        std::vector<Tile*> results;
        for(auto& tile : *layer0) {
            const auto end_coords = tile.GetCoords();
            const bool pLessD = predicate(start_coords, end_coords) < distance;
            const bool dLessP = distance < predicate(start_coords, end_coords);
            if(!pLessD && !dLessP) {
                results.push_back(&tile);
            }
        }
        return results;
    }


    RaycastResult2D StepAndSample(const Vector2& startPosition, const Vector2& endPosition, float sampleRate) const;
    RaycastResult2D StepAndSample(const Vector2& startPosition, const Vector2& direction, float maxDistance, float sampleRate) const;


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
    RaycastResult2D Raycast(const Vector2& startPosition, const Vector2& direction, float maxDistance, bool ignoreSelf, Pr predicate) const {
        const auto endPosition = startPosition + (direction * maxDistance);
        IntVector2 currentTileCoords{startPosition};
        IntVector2 endTileCoords{endPosition};

        const auto D = endPosition - startPosition;

        float tDeltaX = (std::numeric_limits<float>::max)();
        if(!MathUtils::IsEquivalent(D.x, 0.0f)) {
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
        if(!MathUtils::IsEquivalent(D.y, 0.0f)) {
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
                    result.impactSurfaceNormal = Vector2(static_cast<float>(-tileStepX), 0.0f);
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
                    result.impactSurfaceNormal = Vector2(0.0f, static_cast<float>(-tileStepY));
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
    RaycastResult2D Raycast(const Vector2& startPosition, const Vector2& endPosition, bool ignoreSelf, Pr predicate) const {
        const auto displacement = endPosition - startPosition;
        const auto direction = displacement.GetNormalize();
        float length = displacement.CalcLength();
        return Raycast(startPosition, direction, length, ignoreSelf, predicate);
    }
    
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
    IntVector2 TileCoordsFromWorldCoords(const Vector2& worldCoords) const;

    Tile* GetTile(const IntVector3& locationAndLayerIndex) const;
    Tile* GetTile(int x, int y, int z) const;
    Tile* PickTileFromWorldCoords(const Vector2& worldCoords, int layerIndex) const;
    Tile* PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const;

    bool MoveOrAttack(Actor* actor, Tile* tile);

    void SetDebugGridColor(const Rgba& gridColor);

    mutable Camera2D camera{};
    Actor* player = nullptr;

    void KillEntity(Entity& e);
    void KillActor(Actor& a);
    void KillFeature(Feature& f);
    std::vector<Entity*> GetEntities() const noexcept;

    static inline constexpr std::size_t max_layers = 9u;

    void CreateTextEntity(const TextEntityDesc& desc) noexcept;
    void CreateTextEntityAt(const IntVector2& tileCoords, TextEntityDesc desc) noexcept;

    template<typename F>
    void ShakeCamera(F&& f) noexcept {
        camera.trauma = f();
    }

    std::size_t DebugTilesInViewCount() const;
    std::size_t DebugVisibleTilesInViewCount() const;

protected:
private:
    bool LoadFromXML(const XMLElement& elem);
    void LoadNameForMap(const XMLElement& elem);
    void LoadMaterialsForMap(const XMLElement& elem);
    void LoadGenerator(const XMLElement& elem);
    void CreateGeneratorFromTypename(const XMLElement& elem);
    void LoadTileDefinitionsForMap(const XMLElement& elem);
    void LoadTileDefinitionsFromFile(const std::filesystem::path& src);
    void LoadActorsForMap(const XMLElement& elem);
    void LoadFeaturesForMap(const XMLElement& elem);
    void LoadItemsForMap(const XMLElement& elem);

    void UpdateTextEntities(TimeUtils::FPSeconds deltaSeconds);
    void UpdateActorAI(TimeUtils::FPSeconds deltaSeconds);
    void UpdateEntities(TimeUtils::FPSeconds deltaSeconds);

    void BringLayerToFront(std::size_t i);

    void ShakeCamera(const IntVector2& from, const IntVector2& to) noexcept;

    std::string _name{};
    std::vector<std::unique_ptr<Layer>> _layers{};
    Renderer& _renderer;
    Material* _default_tileMaterial{};
    Material* _current_tileMaterial{};
    std::vector<Entity*> _entities{};
    std::vector<EntityText*> _text_entities{};
    std::vector<Actor*> _actors{};
    std::vector<Feature*> _features{};
    std::shared_ptr<SpriteSheet> _tileset_sheet{};
    float _camera_speed = 1.0f;
    mutable std::size_t _debug_tiles_in_view_count{};
    mutable std::size_t _debug_visible_tiles_in_view_count{};
    static inline unsigned long long default_map_index = 0ull;

    friend class MapGenerator;
    friend class HeightMapGenerator;
    friend class FileMapGenerator;
    friend class XmlMapGenerator;
    friend class RoomsMapGenerator;
};
