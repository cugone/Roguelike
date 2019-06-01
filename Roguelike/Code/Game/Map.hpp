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
    std::size_t GetLayerCount() const;
    Layer* GetLayer(std::size_t index) const;
    Tile* GetTile(const IntVector2& location) const;
    Tile* GetTile(int x, int y) const;
    Tile* PickTileFromWorldCoords(const Vector2& worldCoords) const;
    Tile* PickTileFromMouseCoords(const Vector2& mouseCoords) const;
    Vector2 ConvertScreenToWorldCoords(const Vector2& mouseCoords) const;
    void SetDebugGridColor(const Rgba& gridColor);
    mutable Camera2D camera{};

protected:
private:
    bool LoadFromXML(const XMLElement& elem);

    std::string _name{};
    std::vector<std::unique_ptr<Layer>> _layers{};
    Material* _tileMaterial{};
    float _camera_speed = 1.0f;
    static unsigned long long default_map_index;
};
