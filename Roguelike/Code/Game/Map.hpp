#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

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

    Vector2 GetMaxDimensions() const;
    Material* GetTileMaterial() const;
    std::size_t GetLayerCount() const;
    Layer* GetLayer(std::size_t index);
    Tile* GetTile(const IntVector2& location);
    Tile* GetTile(int x, int y);

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
