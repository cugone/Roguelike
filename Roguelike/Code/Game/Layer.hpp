#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Game/Tile.hpp"

class Map;
class Renderer;
class Vector3;

class Layer {
public:
    Layer() = default;
    explicit Layer(Map* map, const XMLElement& elem);
    Layer(const Layer& other) = default;
    Layer(Layer&& other) = default;
    Layer& operator=(const Layer& other) = default;
    Layer& operator=(Layer&& other) = default;
    ~Layer() = default;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(Renderer& renderer) const;
    void DebugRender(Renderer& renderer) const;
    void EndFrame();

    AABB2 CalcOrthoBounds() const;
    AABB2 CalcViewBounds(const Vector2& cam_pos) const;
    AABB2 CalcCullBounds(const Vector2& cam_pos) const;

    const Map* GetMap() const;
    Map* GetMap();
    Tile* GetTile(std::size_t x, std::size_t y);
    Tile* GetTile(std::size_t index);

    int z_index{ 0 };
    float viewHeight{1.0f};
    IntVector2 tileDimensions{1, 1};
    Rgba color{ Rgba::White };
protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    void InitializeTiles(const std::size_t row_count, const std::size_t max_row_length, const std::vector<std::string>& glyph_strings);
    std::size_t NormalizeLayerRows(std::vector<std::string>& glyph_strings);
    void SetModelViewProjectionBounds(Renderer& renderer) const;
    void RenderTiles(Renderer& renderer) const;

    std::vector<Tile> _tiles{};
    Map* _map = nullptr;
};
