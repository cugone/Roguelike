#include "Game/Layer.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Image.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/TileDefinition.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>

Layer::Layer(Map* map, const XMLElement& elem)
    : _map(map)
{
    //TODO: Convert to GUARENTEE_OR_DIE
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Invalid Layer");
    }
}

Layer::Layer(Map* map, const Image& img)
    : _map(map)
{
    //TODO: Convert to GUARENTEE_OR_DIE
    if(!LoadFromImage(img)) {
        ERROR_AND_DIE("Invalid Layer");
    }
}

Layer::Layer(Map* map, const IntVector2& dimensions)
    : _map(map)
    , tileDimensions(dimensions)
{
    const auto layer_width = tileDimensions.x;
    const auto layer_height = tileDimensions.y;
    _tiles.resize(static_cast<std::size_t>(layer_width) * layer_height);
    int x = 0;
    int y = 0;
    for(auto& tile : _tiles) {
        tile.color = Rgba::White;
        tile.layer = this;
        tile.SetCoords(x++, y);
        if(x == layer_width) {
            x = 0;
            ++y;
        }
        if(y == layer_height) {
            y = 0;
        }
    }
}

Tile* Layer::GetNeighbor(const NeighborDirection& direction) {
    switch(direction) {
    case NeighborDirection::Self:
        return GetNeighbor(IntVector2::ZERO);
    case NeighborDirection::East:
        return GetNeighbor(IntVector2::X_AXIS);
    case NeighborDirection::NorthEast:
        return GetNeighbor(IntVector2{1, -1});
    case NeighborDirection::North:
        return GetNeighbor(-IntVector2::Y_AXIS);
    case NeighborDirection::NorthWest:
        return GetNeighbor(-IntVector2::XY_AXIS);
    case NeighborDirection::West:
        return GetNeighbor(-IntVector2::X_AXIS);
    case NeighborDirection::SouthWest:
        return GetNeighbor(IntVector2{-1, 1});
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

void Layer::DirtyMesh() noexcept {
    meshDirty = true;
}

std::vector<Tile>::const_iterator Layer::cbegin() const noexcept {
    return _tiles.cbegin();
}

std::vector<Tile>::const_iterator Layer::cend() const noexcept {
    return _tiles.cend();

}

std::vector<Tile>::reverse_iterator Layer::rbegin() noexcept {
    return _tiles.rbegin();
}

std::vector<Tile>::reverse_iterator Layer::rend() noexcept {
    return _tiles.rend();
}

std::vector<Tile>::const_reverse_iterator Layer::crbegin() const noexcept {
    return _tiles.crbegin();
}

std::vector<Tile>::const_reverse_iterator Layer::crend() const noexcept {
    return _tiles.crend();
}

std::vector<Tile>::iterator Layer::begin() noexcept {
    return _tiles.begin();
}

std::vector<Tile>::iterator Layer::end() noexcept {
    return _tiles.end();
}

const Mesh::Builder& Layer::GetMeshBuilder() const noexcept {
    return _mesh_builder;
}

Mesh::Builder& Layer::GetMeshBuilder() noexcept {
    return const_cast<Mesh::Builder&>(static_cast<const Layer&>(*this).GetMeshBuilder());
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

bool Layer::LoadFromImage(const Image& img) {
    tileDimensions = img.GetDimensions();
    const auto layer_width = tileDimensions.x;
    const auto layer_height = tileDimensions.y;
    _tiles.resize(static_cast<std::size_t>(layer_width) * layer_height);
    int tile_x = 0;
    int tile_y = 0;
    for(auto& t : _tiles) {
        t.color = img.GetTexel(IntVector2{tile_x, tile_y});
        t.SetCoords(tile_x++, tile_y);
        tile_x %= layer_width;
        if(tile_x == 0) {
            ++tile_y;
        }
    }
    return true;
}

void Layer::InitializeTiles(const std::size_t layer_width, const std::size_t layer_height, const std::vector<std::string>& glyph_strings) {
    _tiles.resize(layer_width * layer_height);
    tileDimensions.SetXY(static_cast<int>(layer_width), static_cast<int>(layer_height));
    auto tile_iter = std::begin(_tiles);
    int tile_x = 0;
    int tile_y = 0;
    for(const auto& str : glyph_strings) {
        for(const auto& c : str) {
            tile_iter->ChangeTypeFromGlyph(c);
            tile_iter->SetCoords(tile_x, tile_y);
            tile_iter->layer = this;
            if(auto* def = tile_iter->GetDefinition(); def && def->is_entrance) {
                tile_iter->SetEntrance();
            }
            if(auto* def = tile_iter->GetDefinition(); def && def->is_exit) {
                tile_iter->SetExit();
            }
            ++tile_iter;
            ++tile_x;
        }
        ++tile_y;
        tile_x = 0;
    }
    tile_y = 0;
}

std::size_t Layer::NormalizeLayerRows(std::vector<std::string>& glyph_strings) {
    const auto longest_element = std::max_element(std::cbegin(glyph_strings), std::cend(glyph_strings),
        [](const std::string& a, const std::string& b)->bool {
            return a.size() < b.size();
        });
    const auto longest_row_id = std::size_t{1u} + std::distance(std::cbegin(glyph_strings), longest_element);
    GUARANTEE_OR_DIE(!(Map::max_dimension < longest_element->size()), StringUtils::Stringf("Row %d exceeds maximum length of %d", longest_row_id, Map::max_dimension).c_str());
    const auto max_row_length = (std::min)(longest_element->size(), static_cast<std::size_t>(Map::max_dimension));
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
    const auto leftBottom = Vector2{ortho_bounds.mins.x, ortho_bounds.maxs.y};
    const auto rightTop = Vector2{ortho_bounds.maxs.x, ortho_bounds.mins.y};
    _map->cameraController.GetCamera().SetupView(leftBottom, rightTop, Vector2(0.0f, 1000.0f));
    renderer.SetCamera(_map->cameraController.GetCamera());

    Camera2D& base_camera = _map->cameraController.GetCamera();
    Camera2D shakyCam = _map->cameraController.GetCamera();
    const float shake = shakyCam.GetShake();
    const float shaky_angle = currentGraphicsOptions.MaxShakeAngle * shake * MathUtils::GetRandomFloatNegOneToOne();
    const float shaky_offsetX = currentGraphicsOptions.MaxShakeOffsetHorizontal * shake * MathUtils::GetRandomFloatNegOneToOne();
    const float shaky_offsetY = currentGraphicsOptions.MaxShakeOffsetVertical * shake * MathUtils::GetRandomFloatNegOneToOne();
    shakyCam.orientation_degrees = base_camera.orientation_degrees + shaky_angle;
    shakyCam.position = base_camera.position + Vector2{shaky_offsetX, shaky_offsetY};

    const float cam_rotation_z = shakyCam.GetOrientation();
    const auto VRz = Matrix4::Create2DRotationDegreesMatrix(-cam_rotation_z);

    const auto& cam_pos = shakyCam.GetPosition();
    const auto Vt = Matrix4::CreateTranslationMatrix(-cam_pos);
    const auto v = Matrix4::MakeRT(Vt, VRz);
    renderer.SetViewMatrix(v);

}

void Layer::RenderTiles(Renderer& renderer) const {
    renderer.SetModelMatrix(Matrix4::I);
    Mesh::Render(renderer, _mesh_builder);
}

void Layer::DebugRenderTiles(Renderer& renderer) const {
    renderer.SetModelMatrix(Matrix4::I);

    AABB2 cullbounds = CalcCullBounds(_map->cameraController.GetCamera().position);

    for(auto& t : _tiles) {
        AABB2 tile_bounds = t.GetBounds();
        if(MathUtils::DoAABBsOverlap(cullbounds, tile_bounds)) {
            t.DebugRender(renderer);
        }
    }
}

void Layer::UpdateTiles(TimeUtils::FPSeconds deltaSeconds) {
    debug_tiles_in_view_count = 0;
    debug_visible_tiles_in_view_count = 0;
    const auto& viewableTiles = [this]() {
        const auto view_area = CalcCullBounds(_map->cameraController.GetCamera().GetPosition());
        const auto dims = view_area.CalcDimensions();
        const auto width = static_cast<std::size_t>(dims.x);
        const auto height = static_cast<std::size_t>(dims.y);
        std::vector<Tile*> results;
        results.reserve(width * height);
        for(int x = static_cast<int>(view_area.mins.x); x <= view_area.maxs.x; ++x) {
            if(x >= tileDimensions.x || x < 0) {
                continue;
            }
            for(int y = static_cast<int>(view_area.mins.y); y <= view_area.maxs.y; ++y) {
                if(y >= tileDimensions.y || y < 0) {
                    continue;
                }
                if(auto* tile = GetTile(x, y); tile && (tile->debug_canSee || tile->canSee || tile->haveSeen)) {
                    results.push_back(tile);
                }
            }
        }
        results.shrink_to_fit();
        return results;
    }();
    const auto& visibleTiles = _map->GetVisibleTilesWithinDistance(*_map->player->tile, _map->player->visibility);
    for(auto& tile : visibleTiles) {
        tile->canSee = true;
        tile->haveSeen = true;
    }
    if(meshNeedsRebuild) {
        for(auto& tile : viewableTiles) {
            ++debug_tiles_in_view_count;
            if(tile->canSee || tile->haveSeen) {
                ++debug_visible_tiles_in_view_count;
            }
            tile->AddVerts();
        }
        meshNeedsRebuild = false;
    }
    for(auto& tile : visibleTiles) {
        tile->Update(deltaSeconds);
    }

    if(auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
        if(g_theGame->current_cursor) {
            g_theGame->current_cursor->SetCoords(tile->GetCoords());
            g_theGame->current_cursor->Update(deltaSeconds);
        }
    }
}

void Layer::BeginFrame() {
    for(auto& tile : _tiles) {
        tile.canSee = false;
    }
}

void Layer::Update(TimeUtils::FPSeconds deltaSeconds) {
    UpdateTiles(deltaSeconds);
}

void Layer::Render(Renderer& renderer) const {
    SetModelViewProjectionBounds(renderer);
    RenderTiles(renderer);
}

void Layer::DebugRender(Renderer& renderer) const {
    SetModelViewProjectionBounds(renderer);
    DebugRenderTiles(renderer);
}

void Layer::EndFrame() {
    if(meshDirty) {
        meshNeedsRebuild = true;
        _mesh_builder.Clear();
    }
}

AABB2 Layer::CalcOrthoBounds() const {
    float half_view_height = this->GetMap()->cameraController.GetCamera().GetViewHeight() * 0.5f;
    float half_view_width = half_view_height * _map->cameraController.GetAspectRatio();
    auto ortho_mins = Vector2{-half_view_width, -half_view_height};
    auto ortho_maxs = Vector2{half_view_width, half_view_height};
    return AABB2{ortho_mins, ortho_maxs};
}

AABB2 Layer::CalcViewBounds(const Vector2& cam_pos) const {
    auto view_bounds = CalcOrthoBounds();
    view_bounds.Translate(cam_pos);
    return view_bounds;
}

AABB2 Layer::CalcCullBounds(const Vector2& cam_pos) const {
    auto cullBounds = CalcViewBounds(cam_pos);
    cullBounds.AddPaddingToSides(1.0f, 1.0f);
    return cullBounds;
}

AABB2 Layer::CalcCullBoundsFromOrthoBounds() const {
    auto cullBounds = CalcOrthoBounds();
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
