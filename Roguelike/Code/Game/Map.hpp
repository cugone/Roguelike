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
    Camera2D& GetCamera() const;
    Material* GetTileMaterial() const;
    Layer* GetLayer(std::size_t index);
protected:
private:
    bool LoadFromXML(const XMLElement& elem);
    std::string _name{};
    std::vector<std::unique_ptr<Layer>> _layers{};
    Material* _tileMaterial{};
    mutable Camera2D _camera{};
    float _camera_speed = 1.0f;
    static unsigned long long default_map_index;
};
