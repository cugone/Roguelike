#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Mesh.hpp"

#include "Game/Tile.hpp"

class Image;
class Map;
class Renderer;
class Vector3;

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
    void Render(Renderer& renderer) const;
    void DebugRender(Renderer& renderer) const;
    void EndFrame();

    AABB2 CalcOrthoBounds() const;
    AABB2 CalcViewBounds(const Vector2& cam_pos) const;
    AABB2 CalcCullBounds(const Vector2& cam_pos) const;
    AABB2 CalcCullBoundsFromOrthoBounds() const;

    const Map* GetMap() const;
    Map* GetMap();
    Tile* GetTile(std::size_t x, std::size_t y);
    Tile* GetTile(std::size_t index);

    Tile* GetNeighbor(const NeighborDirection& direction);
    Tile* GetNeighbor(const IntVector2& direction);

    void DirtyMesh() noexcept;

    int z_index{ 0 };
    IntVector2 tileDimensions{1, 1};
    Rgba color{ Rgba::White };
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

    const std::vector<Vertex3D>& GetVbo() const noexcept;
    std::vector<Vertex3D>& GetVbo() noexcept;

    const std::vector<unsigned int>& GetIbo() const noexcept;
    std::vector<unsigned int>& GetIbo() noexcept;
    
protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    bool LoadFromImage(const Image& img);
    void InitializeTiles(const std::size_t row_count, const std::size_t max_row_length, const std::vector<std::string>& glyph_strings);
    std::size_t NormalizeLayerRows(std::vector<std::string>& glyph_strings);
    void SetModelViewProjectionBounds(Renderer& renderer) const;
    void RenderTiles(Renderer& renderer) const;
    void DebugRenderTiles(Renderer& renderer) const;

    void UpdateTiles(TimeUtils::FPSeconds deltaSeconds);

    std::vector<Tile> _tiles{};
    Map* _map = nullptr;
    Mesh::Builder _mesh_builder{};
    std::vector<Vertex3D> vbo{};
    std::vector<unsigned int> ibo{};
    bool meshDirty = true;
    bool meshNeedsRebuild = true;
};
