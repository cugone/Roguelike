#include "Game/Layer.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Image.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector3.hpp"

#include "Game/Game.hpp"
#include "Game/Cursor.hpp"
#include "Game/CursorDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Map.hpp"
#include "Game/Actor.hpp"
#include "Game/Feature.hpp"
#include "Game/TileDefinition.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <tuple>

static std::tuple<Vector2, Vector2, Vector2, Vector2> VertsFromTileCoords(const IntVector2& tile_coords) noexcept;
static std::tuple<Vector2, Vector2, Vector2, Vector2> UVsFromUVCoords(const AABB2& uv_coords) noexcept;


Layer::Layer(Map* map, const XMLElement& elem)
    : m_map(map)
{
    GUARANTEE_OR_DIE(LoadFromXml(elem), "Invalid Layer");
}

Layer::Layer(Map* map, const Image& img)
    : m_map(map)
{
    GUARANTEE_OR_DIE(LoadFromImage(img), "Invalid Layer");
}

Layer::Layer(Map* map, const IntVector2& dimensions)
: m_map(map)
, tileDimensions(dimensions)
{
    const auto layer_width = tileDimensions.x;
    const auto layer_height = tileDimensions.y;
    m_tiles.resize(static_cast<std::size_t>(layer_width) * layer_height);
    for(std::size_t index{0u}; index != m_tiles.size(); ++index) {
        m_tiles[index].layer = this;
        m_tiles[index].SetCoords(index);
    }
}

const Tile* Layer::GetNeighbor(const NeighborDirection& direction) {
    switch(direction) {
    case NeighborDirection::Self:
        return GetNeighbor(IntVector2::Zero);
    case NeighborDirection::East:
        return GetNeighbor(IntVector2::X_Axis);
    case NeighborDirection::NorthEast:
        return GetNeighbor(IntVector2{1, -1});
    case NeighborDirection::North:
        return GetNeighbor(-IntVector2::Y_Axis);
    case NeighborDirection::NorthWest:
        return GetNeighbor(-IntVector2::XY_Axis);
    case NeighborDirection::West:
        return GetNeighbor(-IntVector2::X_Axis);
    case NeighborDirection::SouthWest:
        return GetNeighbor(IntVector2{-1, 1});
    case NeighborDirection::South:
        return GetNeighbor(IntVector2::Y_Axis);
    case NeighborDirection::SouthEast:
        return GetNeighbor(IntVector2::XY_Axis);
    default:
        return nullptr;
    }
}

const Tile* Layer::GetNeighbor(const IntVector2& direction) {
    return GetTile(direction.x, direction.y);
}

void Layer::DirtyMesh() noexcept {
    m_meshDirty = true;
}

std::vector<Tile>::const_iterator Layer::cbegin() const noexcept {
    return m_tiles.cbegin();
}

std::vector<Tile>::const_iterator Layer::cend() const noexcept {
    return m_tiles.cend();

}

std::vector<Tile>::reverse_iterator Layer::rbegin() noexcept {
    return m_tiles.rbegin();
}

std::vector<Tile>::reverse_iterator Layer::rend() noexcept {
    return m_tiles.rend();
}

std::vector<Tile>::const_reverse_iterator Layer::crbegin() const noexcept {
    return m_tiles.crbegin();
}

std::vector<Tile>::const_reverse_iterator Layer::crend() const noexcept {
    return m_tiles.crend();
}

std::vector<Tile>::iterator Layer::begin() noexcept {
    return m_tiles.begin();
}

std::vector<Tile>::iterator Layer::end() noexcept {
    return m_tiles.end();
}

const Mesh::Builder& Layer::GetMeshBuilder() const noexcept {
    return m_mesh_builder;
}

Mesh::Builder& Layer::GetMeshBuilder() noexcept {
    return const_cast<Mesh::Builder&>(static_cast<const Layer&>(*this).GetMeshBuilder());
}

void Layer::DebugShowInvisibleTiles(bool show) noexcept {
    m_showInvisibleTiles = show;
}

void Layer::AppendToMesh(const Tile* const tile) noexcept {
    if(!m_showInvisibleTiles && tile->IsInvisible()) {
        return;
    }
    if(const auto* sprite = [&]()->AnimatedSprite* { if(auto* def = TileDefinition::GetTileDefinitionByName(tile->GetType())) { return def->GetSprite(); } else { return nullptr; } }(); sprite == nullptr) {
        return;
    } else {
        const auto& coords = sprite->GetCurrentTexCoords();
        const auto& tile_coords = tile->GetCoords();
        if(auto* material = sprite->GetMaterial()) {
            AppendToMesh(tile_coords, coords, tile->GetLightValue(), material);
        }
        if(tile->feature) {
            AppendToMesh(tile->feature);
        }
        if(tile->HasInventory()) {
            AppendToMesh(tile->inventory.get(), tile->GetCoords());
        }
        if(tile->actor) {
            AppendToMesh(tile->actor);
        }
    }
}

void Layer::AppendToMesh(const Entity* const entity) noexcept {
    if(!entity || (entity && !entity->sprite) || entity->IsInvisible()) {
        return;
    }
    const auto& coords = entity->sprite->GetCurrentTexCoords();
    const auto& position = entity->GetPosition();
    const auto entity_light_value = [&]() {
        auto evalue = entity->GetLightValue();
        auto tvalue = entity->tile->GetLightValue();
        return (std::max)(evalue, tvalue);
    }(); //IIIL
    entity->AddVertsForCapeEquipment();
    AppendToMesh(position, coords, entity_light_value, entity->sprite->GetMaterial());
    entity->AddVertsForEquipment();
}

void Layer::AppendToMesh(const IntVector2& tile_coords, const AABB2& uv_coords, const uint32_t light_value, Material* material) noexcept {
    const auto&& [vert_bl, vert_tl, vert_tr, vert_br] = VertsFromTileCoords(tile_coords);
    const auto&& [tx_bl, tx_tl, tx_tr, tx_br] = UVsFromUVCoords(uv_coords);

    const float z = static_cast<float>(z_index);
    const Rgba layer_color = color;

    auto& builder = GetMeshBuilder();
    const auto newColor = [&]() {
        auto clr = layer_color != color && color != Rgba::White ? color : layer_color;
        clr.ScaleRGB(MathUtils::RangeMap(static_cast<float>(light_value), static_cast<float>(min_light_value), static_cast<float>(max_light_value), min_light_scale, max_light_scale));
        return clr;
    }(); //IIIL
    const auto normal = -Vector3::Z_Axis;

    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(newColor);
    builder.SetNormal(normal);

    builder.SetUV(tx_bl);
    builder.AddVertex(Vector3{vert_bl, z});

    builder.SetUV(tx_tl);
    builder.AddVertex(Vector3{vert_tl, z});

    builder.SetUV(tx_tr);
    builder.AddVertex(Vector3{vert_tr, z});

    builder.SetUV(tx_br);
    builder.AddVertex(Vector3{vert_br, z});

    builder.AddIndicies(Mesh::Builder::Primitive::Quad);

    builder.End(material);
}

void Layer::AppendToMesh(const Item* const item, const IntVector2& tile_coords) noexcept {
    if(item == nullptr) {
        return;
    }
    const auto* sprite = item->GetSprite();
    if(!sprite) {
        return;
    }
    const auto& uvs = sprite->GetCurrentTexCoords();
    auto* material = sprite->GetMaterial();
    const auto light_value = [&]() {
        if(const auto item_value = item->GetLightValue(); item_value) {
            return item_value;
        } else {
            if(const auto* const tile = GetTile(tile_coords.x, tile_coords.y); tile) {
                return tile->GetLightValue();
            }
        }
        return uint32_t{0u};
    }();
    AppendToMesh(tile_coords, uvs, light_value, material);
}

void Layer::AppendToMesh(const Inventory* const inventory, const IntVector2& tile_coords) noexcept {
    if(inventory && !inventory->empty()) {
        if(const auto* const item = Item::GetItem("chest"); item != nullptr) {
            AppendToMesh(item, tile_coords);
        }
    }
}

void Layer::AppendToMesh(const Cursor* cursor) noexcept {
    if(cursor == nullptr) {
        return;
    }
    const auto&& [vert_bl, vert_tl, vert_tr, vert_br] = VertsFromTileCoords(cursor->GetCoords());

    const auto& sprite = cursor->GetDefinition()->GetSprite();
    const auto& uv_coords = sprite->GetCurrentTexCoords();
    const auto&& [tx_bl, tx_tl, tx_tr, tx_br] = UVsFromUVCoords(uv_coords);

    auto& builder = GetMeshBuilder();
    builder.Begin(PrimitiveType::Triangles);
    builder.SetColor(color);
    builder.SetNormal(-Vector3::Z_Axis);

    builder.SetUV(tx_bl);
    builder.AddVertex(Vector3{vert_bl, 0.0f});

    builder.SetUV(tx_tl);
    builder.AddVertex(Vector3{vert_tl, 0.0f});

    builder.SetUV(tx_tr);
    builder.AddVertex(Vector3{vert_tr, 0.0f});

    builder.SetUV(tx_br);
    builder.AddVertex(Vector3{vert_br, 0.0f});

    builder.AddIndicies(Mesh::Builder::Primitive::Quad);
    builder.End(sprite->GetMaterial());
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
    m_tiles.resize(static_cast<std::size_t>(layer_width) * layer_height);
    int tile_x = 0;
    int tile_y = 0;
    for(auto& t : m_tiles) {
        t.layer = this;
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
    m_tiles.resize(layer_width * layer_height);
    tileDimensions.SetXY(static_cast<int>(layer_width), static_cast<int>(layer_height));
    auto tile_iter = std::begin(m_tiles);
    for(const auto& str : glyph_strings) {
        for(const auto& c : str) {
            tile_iter->layer = this;
            tile_iter->ChangeTypeFromGlyph(c);
            const std::size_t index = std::distance(std::begin(m_tiles), tile_iter);
            tile_iter->SetCoords(static_cast<int>(index % layer_width), static_cast<int>(index / layer_width));
            if(auto* def = TileDefinition::GetTileDefinitionByName(tile_iter->GetType()); def && def->is_entrance) {
                tile_iter->SetEntrance();
            }
            if(auto* def = TileDefinition::GetTileDefinitionByName(tile_iter->GetType()); def && def->is_exit) {
                tile_iter->SetExit();
            }
            ++tile_iter;
        }
    }
}

std::size_t Layer::NormalizeLayerRows(std::vector<std::string>& glyph_strings) {
    const auto longest_element = std::max_element(std::cbegin(glyph_strings), std::cend(glyph_strings),
        [](const std::string& a, const std::string& b)->bool {
            return a.size() < b.size();
        });
    {
        const auto longest_row_id = std::size_t{1u} + std::distance(std::cbegin(glyph_strings), longest_element);
        const std::string err_str = std::format("Row {} exceeds maximum length of {}", longest_row_id, Map::max_dimension);
        GUARANTEE_OR_DIE(!(Map::max_dimension < longest_element->size()), err_str.c_str());
    }
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

void Layer::SetModelViewProjectionBounds() const {

    const auto ortho_bounds = CalcOrthoBounds();

    g_theRenderer->SetModelMatrix(Matrix4::I);
    g_theRenderer->SetViewMatrix(Matrix4::I);
    const auto leftBottom = Vector2{ortho_bounds.mins.x, ortho_bounds.maxs.y};
    const auto rightTop = Vector2{ortho_bounds.maxs.x, ortho_bounds.mins.y};
    m_map->cameraController.GetCamera().SetupView(leftBottom, rightTop, Vector2(0.0f, 1000.0f));
    g_theRenderer->SetCamera(m_map->cameraController.GetCamera());

    Camera2D& base_camera = m_map->cameraController.GetCamera();
    Camera2D shakyCam = m_map->cameraController.GetCamera();
    const float shake = shakyCam.GetShake();
    const float shaky_angle = GetGameAs<Game>()->gameOptions.GetMaxShakeAngle() * shake * MathUtils::GetRandomNegOneToOne<float>();
    const float shaky_offsetX = GetGameAs<Game>()->gameOptions.GetMaxShakeOffsetHorizontal() * shake * MathUtils::GetRandomNegOneToOne<float>();
    const float shaky_offsetY = GetGameAs<Game>()->gameOptions.GetMaxShakeOffsetVertical() * shake * MathUtils::GetRandomNegOneToOne<float>();
    shakyCam.orientation_degrees = base_camera.orientation_degrees + shaky_angle;
    shakyCam.position = base_camera.position + Vector2{shaky_offsetX, shaky_offsetY};

    const float cam_rotation_z = shakyCam.GetOrientation();
    const auto VRz = Matrix4::Create2DRotationDegreesMatrix(-cam_rotation_z);

    const auto& cam_pos = shakyCam.GetPosition();
    const auto Vt = Matrix4::CreateTranslationMatrix(-cam_pos);
    const auto v = Matrix4::MakeRT(Vt, VRz);
    g_theRenderer->SetViewMatrix(v);

}

void Layer::RenderTiles() const {
    g_theRenderer->SetModelMatrix(Matrix4::I);
    Mesh::Render(m_mesh_builder);
}

void Layer::DebugRenderTiles() const {
    g_theRenderer->SetModelMatrix(Matrix4::I);

    AABB2 cullbounds = CalcCullBounds(m_map->cameraController.GetCamera().position);

    for(auto& t : m_tiles) {
        AABB2 tile_bounds = t.GetBounds();
        if(MathUtils::DoAABBsOverlap(cullbounds, tile_bounds)) {
            t.DebugRender();
        }
    }
}

void Layer::UpdateTiles(TimeUtils::FPSeconds deltaSeconds) {
    debug_tiles_in_view_count = 0;
    debug_visible_tiles_in_view_count = 0;
    const auto viewableTiles = [this]() {
        const auto view_area = CalcCullBounds(m_map->cameraController.GetCamera().GetPosition());
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
                if(auto* tile = GetTile(x, y); tile != nullptr) {
                    results.push_back(tile);
                }
            }
        }
        results.shrink_to_fit();
        return results;
    }();
    for(auto& tile : viewableTiles) {
        if(tile->GetLightValue()) {
            tile->SetCanSee();
        }
    }
    const auto& visibleTiles = [&]()->std::vector<Tile*> {
        if(m_map) {
            if(m_map->player && m_map->player->tile) {
                return m_map->GetVisibleTilesWithinDistance(*m_map->player->tile, m_map->player->GetLightValue());
            } else {
                if(const auto* layer = m_map->GetLayer(0); layer != nullptr) {
                    if(Tile* t = m_map->GetTile(layer->tileDimensions.x / 2, layer->tileDimensions.y / 2, 0); t != nullptr) {
                        return m_map->GetVisibleTilesWithinDistance(*t, 1.0f + (std::max)(layer->tileDimensions.x, layer->tileDimensions.y));
                    }
                }
            }
        }
        return {};
    }();
    for(auto& tile : visibleTiles) {
        tile->SetCanSee();
    }
    if(m_meshNeedsRebuild) {
        for(auto& tile : viewableTiles) {
            ++debug_tiles_in_view_count;
            if(tile->CanSee()) {
                ++debug_visible_tiles_in_view_count;
            }
            AppendToMesh(tile);
        }
        m_meshNeedsRebuild = false;
    }
    for(auto& tile : viewableTiles) {
        if(tile->GetLightValue()) {
            tile->Update(deltaSeconds);
        }
    }
}

void Layer::BeginFrame() {
    for(auto& tile : m_tiles) {
        tile.ClearCanSee();
    }
}

void Layer::Update(TimeUtils::FPSeconds deltaSeconds) {
    UpdateTiles(deltaSeconds);
}

void Layer::Render() const {
    SetModelViewProjectionBounds();
    RenderTiles();
}

void Layer::DebugRender() const {
    SetModelViewProjectionBounds();
    DebugRenderTiles();
}

void Layer::EndFrame() {
    if(m_meshDirty) {
        m_meshNeedsRebuild = true;
        m_mesh_builder.Clear();
    }
}

AABB2 Layer::CalcOrthoBounds() const {
    float half_view_height = this->GetMap()->cameraController.GetCamera().GetViewHeight() * 0.5f;
    float half_view_width = half_view_height * m_map->cameraController.GetAspectRatio();
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
    return m_map;
}

Map* Layer::GetMap() {
    return m_map;
}

const Tile* Layer::GetTile(std::size_t x, std::size_t y) const noexcept {
    return GetTile(GetTileIndex(x, y));
}

Tile* Layer::GetTile(std::size_t x, std::size_t y) noexcept {
    return const_cast<Tile*>(static_cast<const Layer&>(*this).GetTile(x, y));
}

const Tile* Layer::GetTile(std::size_t index) const noexcept {
    if(index >= m_tiles.size()) {
        return nullptr;
    }
    return &m_tiles[index];
}

Tile* Layer::GetTile(std::size_t index) noexcept {
    return const_cast<Tile*>(static_cast<const Layer&>(*this).GetTile(index));
}

std::size_t Layer::GetTileIndex(std::size_t x, std::size_t y) const noexcept {
    return x + (y * tileDimensions.x);
}

std::tuple<Vector2, Vector2, Vector2, Vector2> VertsFromTileCoords(const IntVector2& tile_coords) noexcept {
    const auto vert_left = tile_coords.x + 0.0f;
    const auto vert_right = tile_coords.x + 1.0f;
    const auto vert_top = tile_coords.y + 0.0f;
    const auto vert_bottom = tile_coords.y + 1.0f;
    return {Vector2(vert_left, vert_bottom),Vector2(vert_left, vert_top),Vector2(vert_right, vert_top),Vector2(vert_right, vert_bottom)};
}

std::tuple<Vector2, Vector2, Vector2, Vector2> UVsFromUVCoords(const AABB2& uv_coords) noexcept {
    const auto tx_left = uv_coords.mins.x;
    const auto tx_right = uv_coords.maxs.x;
    const auto tx_top = uv_coords.mins.y;
    const auto tx_bottom = uv_coords.maxs.y;
    return {Vector2(tx_left, tx_bottom),Vector2(tx_left, tx_top),Vector2(tx_right, tx_top),Vector2(tx_right, tx_bottom)};
}
