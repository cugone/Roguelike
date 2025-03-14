#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/OrthographicCameraController.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/GameCommon.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/EntityText.hpp"
#include "Game/Inventory.hpp"
#include "Game/Layer.hpp"
#include "Game/MapGenerator.hpp"
#include "Game/Pathfinder.hpp"

#include <filesystem>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <utility>
#include <vector>

class Adventure;
class Material;
class Renderer;
class SpriteSheet;
class Entity;
class Actor;
class TileDefinition;

enum class TimeOfDay {
    Cave
    ,Day
    ,Night
};

struct NewMapOptions {
    std::string name{"NewMap"};
    TimeOfDay time{TimeOfDay::Day};
    IntVector2 dimensions{min_map_width, min_map_height};
    IntVector2 player_start{0,0};
};

class Map {
public:
    constexpr static inline int max_dimension = 255;

    struct RaycastResult2D {
        bool didImpact{false};
        Vector2 impactPosition{};
        std::set<IntVector2> impactTileCoords{};
        float impactFraction{1.0f};
        Vector2 impactSurfaceNormal{};
    };


    Map() noexcept = default;
    explicit Map(IntVector2 dimensions) noexcept;
    explicit Map(const std::filesystem::path& filepath) noexcept;
    explicit Map(const XMLElement& elem) noexcept;
    Map(const Map& other) = default;
    Map(Map&& other) = default;
    Map& operator=(const Map& other) = default;
    Map& operator=(Map&& other) = default;
    ~Map() noexcept;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void FocusCameraOnPlayer(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void UpdateLayers(TimeUtils::FPSeconds deltaSeconds);
    void UpdateCursor(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void AddCursorToTopLayer() noexcept;
    void SetPriorityLayer(std::size_t i);
    void Render() const;
    void DebugRender() const;
    void EndFrame();

    bool IsPlayerOnExit() const noexcept;
    bool IsPlayerOnEntrance() const noexcept;

    bool IsTileInArea(const AABB2& bounds, const IntVector2& tileCoords) const;
    bool IsTileInArea(const AABB2& bounds, const IntVector3& tileCoords) const;
    bool IsTileInArea(const AABB2& bounds, const Tile* tile) const;
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

    bool IsTileEntrance(const IntVector2& tileCoords) const;
    bool IsTileEntrance(const IntVector3& tileCoords) const;
    bool IsTileEntrance(const Tile* tile) const;
    
    bool IsTileExit(const IntVector2& tileCoords) const;
    bool IsTileExit(const IntVector3& tileCoords) const;
    bool IsTileExit(const Tile* tile) const;

    void ZoomOut() noexcept;
    void ZoomIn() noexcept;

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
    std::vector<Tile*> GetViewableTiles() const noexcept;

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
            if(std::invoke(std::forward<Pr>(predicate), start_coords, end_coords) < distance) {
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
            const auto predicateResult = std::invoke(std::forward<Pr>(predicate), start_coords, end_coords);
            const bool pLessD = predicateResult < distance;
            const bool dLessP = distance < predicateResult;
            if(!pLessD && !dLessP) {
                results.push_back(&tile);
            }
        }
        return results;
    }

    void DebugDisableLighting(bool disableLighting) noexcept;
    void DebugShowInvisibleTiles(bool show) noexcept;

    Rgba SkyColor() const noexcept;
    void SetSkyColorToDay() noexcept;
    void SetSkyColorToNight() noexcept;
    void SetSkyColorToCave() noexcept;

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
    RaycastResult2D Raycast(const Vector2& startPosition, const Vector2& direction, float maxDistance, bool ignoreSelf, Pr&& predicate) const {
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
        if(!ignoreSelf && std::invoke(std::forward<Pr>(predicate), currentTileCoords)) {
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
                if(std::invoke(std::forward<Pr>(predicate), currentTileCoords)) {
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
                if(std::invoke(std::forward<Pr>(predicate), currentTileCoords)) {
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
    RaycastResult2D Raycast(const Vector2& startPosition, const Vector2& endPosition, bool ignoreSelf, Pr&& predicate) const {
        const auto displacement = endPosition - startPosition;
        const auto direction = displacement.GetNormalize();
        float length = displacement.CalcLength();
        return Raycast(startPosition, direction, length, ignoreSelf, std::forward<Pr>(predicate));
    }

    AABB2 CalcWorldBounds() const;
    AABB2 CalcCameraBounds() const;
    Vector2 CalcMaxDimensions() const;
    Material* GetTileMaterial() const;
    void SetTileMaterial(Material* material);
    void ResetTileMaterial();
    std::size_t GetLayerCount() const;
    Layer* GetLayer(std::size_t index) const;
    std::optional<std::vector<Tile*>> GetTiles(std::size_t index) const noexcept;
    std::optional<std::vector<Tile*>> GetTiles(const IntVector2& location) const;
    std::optional<std::vector<Tile*>> GetTiles(int x, int y) const;
    std::size_t ConvertLocationToIndex(int x, int y) const noexcept;
    std::size_t ConvertLocationToIndex(const IntVector2& location) const noexcept;
    IntVector2 ConvertIndexToLocation(std::size_t index) const noexcept;
    std::optional<std::vector<Tile*>> PickTilesFromWorldCoords(const Vector2& worldCoords) const;
    std::optional<std::vector<Tile*>> PickTilesFromMouseCoords(const Vector2& mouseCoords) const;
    Vector2 WorldCoordsToScreenCoords(const Vector2& worldCoords) const;
    Vector2 ScreenCoordsToWorldCoords(const Vector2& screenCoords) const;
    IntVector2 TileCoordsFromWorldCoords(const Vector2& worldCoords) const;

    Tile* GetTile(const IntVector3& locationAndLayerIndex) const;
    Tile* GetTile(int x, int y, int z) const;
    Tile* PickTileFromWorldCoords(const Vector2& worldCoords, int layerIndex) const;
    Tile* PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const;

    Vector2 GetSubTileLocationFromMouseCoords(const Vector2& mouseCoords) const noexcept;

    bool MoveOrAttack(Actor* actor, Tile* tile);

    void SetDebugGridColor(const Rgba& gridColor);
    void SetDebugGlobalLight(uint32_t lightValue);
    int GetCurrentGlobalLightValue() const noexcept;

    OrthographicCameraController cameraController{};
    Actor* player = nullptr;

    void KillEntity(Entity& e);
    void KillActor(Actor& a);
    void KillFeature(Feature& f);
    const std::vector<Entity*>& GetEntities() const noexcept;
    const std::vector<EntityText*>& GetTextEntities() const noexcept;

    static inline constexpr std::size_t max_layers = 9u;

    void CreateTextEntity(const TextEntityDesc& desc) noexcept;
    void CreateTextEntityAt(const IntVector2& tileCoords, TextEntityDesc desc) noexcept;

    std::size_t DebugTilesInViewCount() const;
    std::size_t DebugVisibleTilesInViewCount() const;

    void GenerateMap(const XMLElement& elem) noexcept;
    void RegenerateMap() noexcept;

    const Pathfinder* GetPathfinder() const noexcept;
    Pathfinder* GetPathfinder() noexcept;
    
    void DirtyTileLight(TileInfo& ti) noexcept;

    MapGenerator _map_generator;

protected:
private:
    void Initialize(const XMLElement& elem) noexcept;
    void SetParentAdventure(Adventure* parent) noexcept;

    bool LoadFromTmx(const XMLElement& elem);
    bool LoadFromXML(const XMLElement& elem);
    void LoadTimeOfDayForMap(const XMLElement& elem);
    void LoadNameForMap(const XMLElement& elem);
    void LoadMaterialsForMap(const XMLElement& elem);
    void LoadGenerator(const XMLElement& elem);

    void LoadTileDefinitionsForMap(const XMLElement& elem);
    void LoadMaterialFromFile(const std::filesystem::path& src) noexcept;
    void LoadActorsForMap(const XMLElement& elem);
    void LoadFeaturesForMap(const XMLElement& elem);
    void LoadItemsForMap(const XMLElement& elem);

    bool AllowLightingDuringDay() const noexcept;
    void InitializeLighting(Layer* layer) noexcept;
    void CalculateLighting(Layer* layer) noexcept;
    void DirtyValidNeighbors(TileInfo& ti) noexcept;
    void DirtyCardinalNeighbors(TileInfo& ti) noexcept;
    void UpdateTileLighting(TileInfo& ti) noexcept;
    void DirtyNeighborLighting(TileInfo& ti, const Layer::NeighborDirection& direction) noexcept;

    void UpdateTextEntities(TimeUtils::FPSeconds deltaSeconds);
    void UpdateActorAI(TimeUtils::FPSeconds deltaSeconds);
    void UpdateEntities(TimeUtils::FPSeconds deltaSeconds);
    void UpdateLighting(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void CalculateLightingForLayers([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;

    void SetCursorForFaction(const Actor* actor) const noexcept;
    void SetCursorForTile() const noexcept;
    void ShouldRenderStatWindow() noexcept;
    void RenderClayStatsBlock() const noexcept;

    void BringLayerToFront(std::size_t i);

    void ShakeCamera(const IntVector2& from, const IntVector2& to) noexcept;

    void SetGlobalLightFromSkyColor() noexcept;
    void SetSkyColorFromGlobalLight() noexcept;

    static const Rgba& GetSkyColorForDay() noexcept;
    static const Rgba& GetSkyColorForNight() noexcept;
    static const Rgba& GetSkyColorForCave() noexcept;

    void SetCustomSkyColor(const Rgba& newColor) noexcept;

    std::pair<int, std::filesystem::path> ParseTmxTilesetElement(const XMLElement& elem) noexcept;
    bool ParseTmxTileLayerElements(const XMLElement& elem, int firstgid) noexcept;
    void InitializeTilesFromTmxData(Layer* layer, const XMLElement& elem, int firstgid) noexcept;
    void LoadTmxTileset(const XMLElement& elem) noexcept;

    std::string _name{};
    std::filesystem::path m_filepath{};
    std::vector<std::shared_ptr<Layer>> _layers{};
    std::deque<TileInfo> _lightingQueue{};
    std::shared_ptr<tinyxml2::XMLDocument> _xml_doc{};
    XMLElement* _root_xml_element{};
    Adventure* _parent_adventure{};
    Material* _default_tileMaterial{};
    Material* _current_tileMaterial{};
    Rgba _current_sky_color{};
    uint32_t _current_global_light{};
    Pathfinder _pathfinder{};
    std::vector<Entity*> _entities{};
    std::vector<EntityText*> _text_entities{};
    std::vector<Actor*> _actors{};
    std::vector<Feature*> _features{};
    std::shared_ptr<SpriteSheet> _tileset_sheet{};

    float _camera_speed = 1.0f;
    mutable std::size_t _debug_tiles_in_view_count{};
    mutable std::size_t _debug_visible_tiles_in_view_count{};
    static inline unsigned long long default_map_index = 0ull;
    bool _should_render_stat_window{false};
    bool _allow_lighting_calculations_during_day{true};
    bool m_isInfinite{ false };
    uint16_t m_chunkWidth{ 16u };
    uint16_t m_chunkHeight{ 16u };
    friend class MapGenerator;
    friend class HeightMapGenerator;
    friend class FileMapGenerator;
    friend class XmlMapGenerator;
    friend class RoomsMapGenerator;
    friend class RoomsOnlyMapGenerator;
    friend class RoomsAndCorridorsMapGenerator;
    friend class Adventure;
    friend class Game;
    friend class MapEditor;
    friend class TmxReader;
    friend class TsxReader;
};
