#include "Game/Layer.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Map.hpp"

#include <algorithm>
#include <numeric>

Layer::Layer(Map* map, const XMLElement& elem)
    : _map(map)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Invalid Layer");
    }
}

Tile* Layer::GetNeighbor(const NeighborDirection& direction) {
    switch(direction) {
    case NeighborDirection::Self:
        return GetNeighbor(IntVector2::ZERO);
    case NeighborDirection::East:
        return GetNeighbor(IntVector2::X_AXIS);
    case NeighborDirection::NorthEast:
        return GetNeighbor(IntVector2{ -1, 1 });
    case NeighborDirection::North:
        return GetNeighbor(-IntVector2::Y_AXIS);
    case NeighborDirection::NorthWest:
        return GetNeighbor(-IntVector2::XY_AXIS);
    case NeighborDirection::West:
        return GetNeighbor(-IntVector2::X_AXIS);
    case NeighborDirection::SouthWest:
        return GetNeighbor(IntVector2{ -1, 1 });
    case NeighborDirection::South:
        return GetNeighbor(IntVector2::Y_AXIS);
    case NeighborDirection::SouthEast:
        return GetNeighbor(IntVector2::XY_AXIS);
    default:
        return nullptr;
    }
}

Tile* Layer::GetNeighbor(const IntVector2& direction) {
    return GetTile(direction.x, direction.y);
}

float Layer::GetDefaultViewHeight() const {
    return _defaultViewHeight;
}

bool Layer::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "layer", "row", "");
    std::size_t row_count = DataUtils::GetChildElementCount(elem, "row");
    std::vector<std::string> glyph_strings;
    glyph_strings.reserve(row_count);
    DataUtils::ForEachChildElement(elem, "row",
    [this, &glyph_strings](const XMLElement& elem) {
        DataUtils::ValidateXmlElement(elem, "row", "", "glyphs");
        auto glyph_str = DataUtils::ParseXmlAttribute(elem, "glyphs", std::string{});
        glyph_strings.push_back(glyph_str);
    });
    auto max_row_length = NormalizeLayerRows(glyph_strings);
    InitializeTiles(max_row_length, row_count, glyph_strings);
    return true;
}

void Layer::InitializeTiles(const std::size_t layer_width, const std::size_t layer_height, const std::vector<std::string>& glyph_strings) {
    _tiles.resize(layer_width * layer_height);
    tileDimensions.SetXY(static_cast<int>(layer_width), static_cast<int>(layer_height));
    viewHeight = static_cast<float>(layer_height);
    _defaultViewHeight = viewHeight;
    auto tile_iter = std::begin(_tiles);
    int tile_x = 0;
    int tile_y = 0;
    for(const auto& str : glyph_strings) {
        for(const auto& c : str) {
            tile_iter->ChangeTypeFromGlyph(c);
            tile_iter->SetCoords(tile_x, tile_y);
            tile_iter->layer = this;
            ++tile_iter;
            ++tile_x;
        }
        ++tile_y;
        tile_x = 0;
    }
    tile_y = 0;
}

std::size_t Layer::NormalizeLayerRows(std::vector<std::string>& glyph_strings) {
    const auto longest_element = std::max_element(std::begin(glyph_strings), std::end(glyph_strings),
        [](const std::string& a, const std::string& b)->bool {
        return a.size() < b.size();
    });
    const auto max_row_length = longest_element->size();
    for(auto& str : glyph_strings) {
        if(str.empty()) {
            str.assign(max_row_length, ' ');
        }
        if(str.size() < max_row_length) {
            const auto delta_row_length = max_row_length - str.size();
            if(delta_row_length < 2) {
                str.append(1, ' ');
            } else {
                str.append(delta_row_length, ' ');
            }
            str.shrink_to_fit();
        }
    }
    return max_row_length;
}

void Layer::SetModelViewProjectionBounds(Renderer& renderer) const {

    const auto ortho_bounds = CalcOrthoBounds();

    renderer.SetModelMatrix(Matrix4::I);
    renderer.SetViewMatrix(Matrix4::I);
    const auto leftBottom = Vector2{ ortho_bounds.mins.x, ortho_bounds.maxs.y };
    const auto rightTop = Vector2{ ortho_bounds.maxs.x, ortho_bounds.mins.y };
    _map->camera.SetupView(leftBottom, rightTop, Vector2(0.0f, 1000.0f));
    renderer.SetCamera(_map->camera);

    Camera2D& base_camera = _map->camera;
    Camera2D shakyCam = _map->camera;
    const float shake = shakyCam.GetShake();
    const float shaky_angle = currentGraphicsOptions.MaxShakeAngle * shake * MathUtils::GetRandomFloatNegOneToOne();
    const float shaky_offsetX = currentGraphicsOptions.MaxShakeOffsetHorizontal * shake * MathUtils::GetRandomFloatNegOneToOne();
    const float shaky_offsetY = currentGraphicsOptions.MaxShakeOffsetVertical * shake * MathUtils::GetRandomFloatNegOneToOne();
    shakyCam.orientation_degrees = base_camera.orientation_degrees + shaky_angle;
    shakyCam.position = base_camera.position + Vector2{ shaky_offsetX, shaky_offsetY };

    const float cam_rotation_z = shakyCam.GetOrientation();
    const auto VRz = Matrix4::Create2DRotationDegreesMatrix(-cam_rotation_z);

    const auto cam_pos = shakyCam.GetPosition();
    const auto Vt = Matrix4::CreateTranslationMatrix(-cam_pos);
    const auto v = Matrix4::MakeRT(Vt, VRz);
    renderer.SetViewMatrix(v);

}

void Layer::RenderTiles(Renderer& renderer) const {
    renderer.SetModelMatrix(Matrix4::I);

    AABB2 cullbounds = CalcCullBounds(_map->camera.position);

    static std::vector<Vertex3D> verts;
    verts.clear();
    static std::vector<unsigned int> ibo;
    ibo.clear();

    for(auto& t : _tiles) {
        AABB2 tile_bounds = t.GetBounds();
        if(MathUtils::DoAABBsOverlap(cullbounds, tile_bounds)) {
            t.Render(verts, ibo, color, z_index);
        }
    }
    renderer.SetMaterial(_map->GetTileMaterial());
    renderer.DrawIndexed(PrimitiveType::Triangles, verts, ibo);
}

void Layer::DebugRenderTiles(Renderer& renderer) const {
    renderer.SetModelMatrix(Matrix4::I);

    AABB2 cullbounds = CalcCullBounds(_map->camera.position);

    static std::vector<Vertex3D> verts;
    verts.clear();
    static std::vector<unsigned int> ibo;
    ibo.clear();

    for(auto& t : _tiles) {
        AABB2 tile_bounds = t.GetBounds();
        if(MathUtils::DoAABBsOverlap(cullbounds, tile_bounds)) {
            t.DebugRender(renderer);
        }
    }
    renderer.SetMaterial(_map->GetTileMaterial());
    renderer.DrawIndexed(PrimitiveType::Triangles, verts, ibo);
}

void Layer::UpdateTiles(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& tile : _tiles) {
        tile.Update(deltaSeconds);
    }
}

void Layer::BeginFrame() {
    /* DO NOTHING */
}

void Layer::Update(TimeUtils::FPSeconds deltaSeconds) {
    UpdateTiles(deltaSeconds);
}

void Layer::Render(Renderer& renderer) const {
    SetModelViewProjectionBounds(renderer);
    RenderTiles(renderer);
}

void Layer::DebugRender(Renderer& renderer) const {
    DebugRenderTiles(renderer);
}

void Layer::EndFrame() {
    /* DO NOTHING */
}

AABB2 Layer::CalcOrthoBounds() const {
    float half_view_height = viewHeight * 0.5f;
    float half_view_width = half_view_height * _map->camera.GetAspectRatio();
    auto ortho_mins = Vector2{ -half_view_width, -half_view_height };
    auto ortho_maxs = Vector2{ half_view_width, half_view_height };
    return AABB2{ ortho_mins, ortho_maxs };
}

AABB2 Layer::CalcViewBounds(const Vector2& cam_pos) const {
    auto view_bounds = CalcOrthoBounds();
    view_bounds.Translate(cam_pos);
    return view_bounds;
}

AABB2 Layer::CalcCullBounds(const Vector2& cam_pos) const {
    AABB2 cullBounds = CalcViewBounds(cam_pos);
    cullBounds.AddPaddingToSides(1.0f, 1.0f);
    return cullBounds;
}

const Map* Layer::GetMap() const {
    return _map;
}

Map* Layer::GetMap() {
    return _map;
}

Tile* Layer::GetTile(std::size_t x, std::size_t y) {
    return GetTile(x + (y * tileDimensions.x));
}

Tile* Layer::GetTile(std::size_t index) {
    if(index >= _tiles.size()) {
        return nullptr;
    }
    return &_tiles[index];
}
