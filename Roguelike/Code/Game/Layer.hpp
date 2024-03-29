#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Mesh.hpp"

#include "Game/Tile.hpp"

class Image;
class Renderer;
class Vector3;
class Map;
class Cursor;

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
    explicit Layer(Map* map, const IntVector2& dimensions);
    explicit Layer(Map* map, const XMLElement& elem);
    explicit Layer(Map* map, const Image& img);
    Layer(const Layer& other) = default;
    Layer(Layer&& other) = default;
    Layer& operator=(const Layer& other) = default;
    Layer& operator=(Layer&& other) = default;
    ~Layer() = default;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render() const;
    void DebugRender() const;
    void EndFrame();

    AABB2 CalcOrthoBounds() const;
    AABB2 CalcViewBounds(const Vector2& cam_pos) const;
    AABB2 CalcCullBounds(const Vector2& cam_pos) const;
    AABB2 CalcCullBoundsFromOrthoBounds() const;

    const Map* GetMap() const;
    Map* GetMap();
    const Tile* GetTile(std::size_t x, std::size_t y) const noexcept;
    Tile* GetTile(std::size_t x, std::size_t y) noexcept;
    const Tile* GetTile(std::size_t index) const noexcept;
    Tile* GetTile(std::size_t index) noexcept;
    std::size_t GetTileIndex(std::size_t x, std::size_t y) const noexcept;

    const Tile* GetNeighbor(const NeighborDirection& direction);
    const Tile* GetNeighbor(const IntVector2& direction);

    void DirtyMesh() noexcept;

    int z_index{0};
    IntVector2 tileDimensions{1, 1};
    Rgba color{Rgba::White};
    Rgba debug_grid_color{Rgba::Red};
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

    const Mesh::Builder& GetMeshBuilder() const noexcept;
    Mesh::Builder& GetMeshBuilder() noexcept;

    void DebugShowInvisibleTiles(bool show) noexcept;

    void AppendToMesh(const Tile* const tile) noexcept;
    void AppendToMesh(const Entity* const entity) noexcept;
    void AppendToMesh(const Item* const item, const IntVector2& tile_coords) noexcept;
    void AppendToMesh(const Inventory* const inventory, const IntVector2& tile_coords) noexcept;
    void AppendToMesh(const IntVector2& tile_coords, const AABB2& uv_coords, const uint32_t light_value, Material* material) noexcept;
    void AppendToMesh(const Cursor* cursor) noexcept;

protected:
private:

    bool LoadFromXml(const XMLElement& elem);
    bool LoadFromImage(const Image& img);
    void InitializeTiles(const std::size_t row_count, const std::size_t max_row_length, const std::vector<std::string>& glyph_strings);
    std::size_t NormalizeLayerRows(std::vector<std::string>& glyph_strings);
    void SetModelViewProjectionBounds() const;
    void RenderTiles() const;
    void DebugRenderTiles() const;

    void UpdateTiles(TimeUtils::FPSeconds deltaSeconds);

    std::vector<Tile> m_tiles{};
    Map* m_map = nullptr;
    Mesh::Builder m_mesh_builder{};
    bool m_meshDirty = true;
    bool m_meshNeedsRebuild = true;
    bool m_showInvisibleTiles = false;
};
