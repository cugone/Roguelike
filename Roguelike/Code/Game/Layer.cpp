#include "Game/Layer.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"

#include "Engine/Math/Vector3.hpp"

#include "Game/GameCommon.hpp"
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

bool Layer::LoadFromXml(const XMLElement& elem) {
    //Load Layer from Xml
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
    auto tile_iter = std::begin(_tiles);
    int tile_x = 0;
    int tile_y = 0;
    for(const auto& str : glyph_strings) {
        for(const auto& c : str) {
            tile_iter->ChangeTypeFromGlyph(c);
            tile_iter->SetCoords(tile_x, tile_y);
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

    auto ortho_bounds = CalcOrthoBounds();

    renderer.SetModelMatrix(Matrix4::GetIdentity());
    renderer.SetViewMatrix(Matrix4::GetIdentity());
    auto leftBottom = Vector2{ ortho_bounds.mins.x, ortho_bounds.maxs.y };
    auto rightTop = Vector2{ ortho_bounds.maxs.x, ortho_bounds.mins.y };
    renderer.SetOrthoProjection(leftBottom, rightTop, Vector2(0.0f, 1000.0f));

    float cam_rotation_z = g_theGame->GetCamera().GetOrientation();
    auto VRz = Matrix4::Create2DRotationDegreesMatrix(-cam_rotation_z);

    auto cam_pos = g_theGame->GetCamera().GetPosition();
    auto Vt = Matrix4::CreateTranslationMatrix(-cam_pos);
    auto v = VRz * Vt;
    renderer.SetViewMatrix(v);

}

void Layer::RenderTiles(Renderer& renderer) const {
    renderer.SetModelMatrix(Matrix4::GetIdentity());

    AABB2 cullbounds = CalcCullBounds(g_theGame->GetCamera().GetPosition());

    static std::vector<Vertex3D> verts;
    verts.clear();

    for(auto& t : _tiles) {
        AABB2 tile_bounds = t.GetBounds();
        if(MathUtils::DoAABBsOverlap(cullbounds, tile_bounds)) {
            t.Render(verts, color, z_index);
        }
    }

    std::vector<unsigned int> ibo(verts.size());
    std::iota(ibo.begin(), ibo.end(), 0); //Fill ibo from 0 to size - 1

    renderer.SetMaterial(_map->GetTileMaterial());
    renderer.DrawIndexed(PrimitiveType::Triangles, verts, ibo);
}

void Layer::BeginFrame() {

}

void Layer::Update(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& tile : _tiles) {
        tile.Update(deltaSeconds);
    }
}

void Layer::Render(Renderer& renderer) const {
    SetModelViewProjectionBounds(renderer);
    RenderTiles(renderer);
}

void Layer::DebugRender(Renderer& renderer) const {
    renderer.SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2::ONE * -1.0f));
    renderer.DrawWorldGrid2D(tileDimensions, Rgba::Black);
}

void Layer::EndFrame() {

}

AABB2 Layer::CalcOrthoBounds() const {
    float half_view_height = viewHeight * 0.5f;
    float half_view_width = half_view_height * g_theGame->GetCamera().GetAspectRatio();
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
    return const_cast<Map*>(static_cast<const Layer&>(*this).GetMap());
}

Tile* Layer::GetTile(std::size_t x, std::size_t y) {
    return GetTile(x + (y * tileDimensions.x));
}

Tile* Layer::GetTile(std::size_t index) {
    if(index >= static_cast<std::size_t>(tileDimensions.x * tileDimensions.y)) {
        return nullptr;
    }
    return &_tiles[index];
}
