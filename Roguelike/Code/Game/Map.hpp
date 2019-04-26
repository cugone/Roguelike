#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

class Material;
class Layer;
class Renderer;

class Map {
public:
    Map() = default;
    Map(const XMLElement& elem);
    Map(const Map& other) = default;
    Map(Map&& other) = default;
    Map& operator=(const Map& other) = default;
    Map& operator=(Map&& other) = default;
    ~Map() = default;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(Renderer& renderer) const;
    void EndFrame();

protected:
private:
    bool LoadFromXML(const XMLElement& elem);

    std::vector<Layer*> _layers{};
    Material* _tileMaterial = nullptr;

};
