#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include <memory>

#include "Game/Layer.hpp"

class Material;
class Renderer;
class TileDefinition;

class Map {
public:
    Map() = default;
    explicit Map(const XMLElement& elem);
    Map(const Map& other) = default;
    Map(Map&& other) = default;
    Map& operator=(const Map& other) = default;
    Map& operator=(Map&& other) = default;
    ~Map() = default;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(Renderer& renderer) const;
    void DebugRender(Renderer& renderer) const;
    void EndFrame();

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
    Vector2 ConvertScreenToWorldCoords(const Vector2& mouseCoords) const;
    void SetDebugGridColor(const Rgba& gridColor);
    mutable Camera2D camera{};

protected:
private:
    bool LoadFromXML(const XMLElement& elem);

    std::string _name{};
    std::vector<std::unique_ptr<Layer>> _layers{};
    Material* _default_tileMaterial{};
    Material* _current_tileMaterial{};
    float _camera_speed = 1.0f;
    static unsigned long long default_map_index;
};
