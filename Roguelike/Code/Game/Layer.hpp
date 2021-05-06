#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Mesh.hpp"

#include "Game/Tile.hpp"

namespace a2de {
    class Image;
    class Renderer;
    class Vector3;
}

class Map;

class Layer {
public:
    enum class NeighborDirection {
        Self,
        East,
        NorthEast,
        North,
        NorthWest,
        West,
        SouthWest,
        South,
        SouthEast,
    };

    Layer() = default;
    explicit Layer(Map* map, const a2de::IntVector2& dimensions);
    explicit Layer(Map* map, const a2de::XMLElement& elem);
    explicit Layer(Map* map, const a2de::Image& img);
    Layer(const Layer& other) = default;
    Layer(Layer&& other) = default;
    Layer& operator=(const Layer& other) = default;
    Layer& operator=(Layer&& other) = default;
    ~Layer() = default;

    void BeginFrame();
    void Update(a2de::TimeUtils::FPSeconds deltaSeconds);
    void Render(a2de::Renderer& renderer) const;
    void DebugRender(a2de::Renderer& renderer) const;
    void EndFrame();

    a2de::AABB2 CalcOrthoBounds() const;
    a2de::AABB2 CalcViewBounds(const a2de::Vector2& cam_pos) const;
    a2de::AABB2 CalcCullBounds(const a2de::Vector2& cam_pos) const;
    a2de::AABB2 CalcCullBoundsFromOrthoBounds() const;

    const Map* GetMap() const;
    Map* GetMap();
    Tile* GetTile(std::size_t x, std::size_t y);
    Tile* GetTile(std::size_t index);

    Tile* GetNeighbor(const NeighborDirection& direction);
    Tile* GetNeighbor(const a2de::IntVector2& direction);

    void DirtyMesh() noexcept;

    int z_index{ 0 };
    a2de::IntVector2 tileDimensions{1, 1};
    a2de::Rgba color{a2de::Rgba::White };
    a2de::Rgba debug_grid_color{a2de::Rgba::Red};
    std::size_t debug_tiles_in_view_count{};
    std::size_t debug_visible_tiles_in_view_count{};

    std::vector<Tile>::const_iterator cbegin() const noexcept;
    std::vector<Tile>::const_iterator cend() const noexcept;
    std::vector<Tile>::reverse_iterator rbegin() noexcept;
    std::vector<Tile>::reverse_iterator rend() noexcept;
    std::vector<Tile>::const_reverse_iterator crbegin() const noexcept;
    std::vector<Tile>::const_reverse_iterator crend() const noexcept;
    std::vector<Tile>::iterator begin() noexcept;
    std::vector<Tile>::iterator end() noexcept;

    const a2de::Mesh::Builder& GetMeshBuilder() const noexcept;
    a2de::Mesh::Builder& GetMeshBuilder() noexcept;

protected:
private:
    bool LoadFromXml(const a2de::XMLElement& elem);
    bool LoadFromImage(const a2de::Image& img);
    void InitializeTiles(const std::size_t row_count, const std::size_t max_row_length, const std::vector<std::string>& glyph_strings);
    std::size_t NormalizeLayerRows(std::vector<std::string>& glyph_strings);
    void SetModelViewProjectionBounds(a2de::Renderer& renderer) const;
    void RenderTiles(a2de::Renderer& renderer) const;
    void DebugRenderTiles(a2de::Renderer& renderer) const;

    void UpdateTiles(a2de::TimeUtils::FPSeconds deltaSeconds);

    std::vector<Tile> _tiles{};
    Map* _map = nullptr;
    a2de::Mesh::Builder _mesh_builder{};
    bool meshDirty = true;
    bool meshNeedsRebuild = true;
};
