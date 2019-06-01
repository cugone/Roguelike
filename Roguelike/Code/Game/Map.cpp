#include "Game/Map.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"

#include "Engine/Math/Vector4.hpp"

#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Game/GameCommon.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Layer.hpp"

#include <sstream>

unsigned long long Map::default_map_index = 0ull;

Vector2 Map::ConvertScreenToWorldCoords(const Vector2& mouseCoords) const {
    auto ndc = 2.0f * mouseCoords / Vector2(g_theRenderer->GetOutput()->GetDimensions()) - Vector2::ONE;
    auto screenCoords4 = Vector4(ndc.x, -ndc.y, 1.0f, 1.0f);
    auto sToW = camera.GetInverseViewProjectionMatrix();
    auto worldPos4 = sToW * screenCoords4;
    auto worldPos3 = Vector3(worldPos4);
    auto worldPos2 = Vector2(worldPos3);
    return worldPos2;
}

void Map::SetDebugGridColor(const Rgba& gridColor) {
    for(auto& layer : _layers) {
        layer->debug_grid_color = gridColor;
    }
}

AABB2 Map::CalcWorldBounds() const {
    return {Vector2::ZERO, CalcMaxDimensions()};
}

Tile* Map::PickTileFromWorldCoords(const Vector2& worldCoords) const {
    auto world_bounds = CalcWorldBounds();
    if(MathUtils::IsPointInside(world_bounds, worldCoords)) {
        return GetTile(IntVector2{ worldCoords });
    }
    return nullptr;
}

Tile* Map::PickTileFromMouseCoords(const Vector2& mouseCoords) const {
    auto world_coords = ConvertScreenToWorldCoords(mouseCoords);
    return PickTileFromWorldCoords(world_coords);
}

Map::Map(const XMLElement& elem) {
    if(!LoadFromXML(elem)) {
        ERROR_AND_DIE("Could not load map.");
    }
}

void Map::BeginFrame() {
    for(auto& layer : _layers) {
        layer->BeginFrame();
    }
}

void Map::Update(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& layer : _layers) {
        layer->Update(deltaSeconds);
    }
}

void Map::Render(Renderer& renderer) const {
    for(const auto& layer : _layers) {
        layer->Render(renderer);
    }
}

void Map::DebugRender(Renderer& renderer) const {
    for(const auto& layer : _layers) {
        layer->DebugRender(renderer);
    }
    if(g_theGame->_show_world_bounds) {
        auto bounds = CalcWorldBounds();
        renderer.SetModelMatrix(Matrix4::I);
        renderer.DrawAABB2(bounds, Rgba::Cyan, Rgba::NoAlpha);
    }
}

void Map::EndFrame() {
    for(auto& layer : _layers) {
        layer->EndFrame();
    }
}

Vector2 Map::CalcMaxDimensions() const {
    Vector2 results{ 1.0f, 1.0f };
    for(const auto& layer : _layers) {
        const auto& cur_layer_dimensions = layer->tileDimensions;
        if(cur_layer_dimensions.x > results.x) {
            results.x = static_cast<float>(cur_layer_dimensions.x);
        }
        if(cur_layer_dimensions.y > results.y) {
            results.y = static_cast<float>(cur_layer_dimensions.y);
        }
    }
    return results;
}

float Map::CalcMaxViewHeight() const {
    auto max_view_height_elem = std::max_element(std::begin(_layers), std::end(_layers),
    [](const std::unique_ptr<Layer>& a, const std::unique_ptr<Layer>& b)->bool{
        return a->viewHeight < b->viewHeight;
    });
    return (*max_view_height_elem)->viewHeight;
}

Material* Map::GetTileMaterial() const {
    return _tileMaterial;
}

std::size_t Map::GetLayerCount() const {
    return _layers.size();
}

Layer* Map::GetLayer(std::size_t index) const {
    if(index >= _layers.size()) {
        return nullptr;
    }
    return _layers[index].get();
}

Tile* Map::GetTile(const IntVector2& location) const {
    return GetTile(location.x, location.y);
}

Tile* Map::GetTile(int x, int y) const {
    return GetLayer(0)->GetTile(x, y);
}

bool Map::LoadFromXML(const XMLElement& elem) {

    DataUtils::ValidateXmlElement(elem, "map", "tiles,layers,material", "name");

    {
        std::ostringstream ss;
        ss << "MAP " << ++default_map_index;
        std::string default_name = ss.str();
        _name = DataUtils::ParseXmlAttribute(elem, "name", default_name);
    }

    if(auto xml_material = elem.FirstChildElement("material")) {
        DataUtils::ValidateXmlElement(*xml_material, "material", "", "name");
        auto src = DataUtils::ParseXmlAttribute(*xml_material, "name", std::string{"__invalid"});
        _tileMaterial = g_theRenderer->GetMaterial(src);
    }

    if(auto xml_tileset = elem.FirstChildElement("tiles")) {
        DataUtils::ValidateXmlElement(*xml_tileset, "tiles", "", "src");
        auto src = DataUtils::ParseXmlAttribute(*xml_tileset, "src", std::string{});
        if(src.empty()) {
            ERROR_AND_DIE("Map tiles source is empty.");
        }
        tinyxml2::XMLDocument doc;
        auto xml_result = doc.LoadFile(src.c_str());
        if(xml_result != tinyxml2::XML_SUCCESS) {
            std::ostringstream ss;
            ss << "Map at " << src << " failed to load.";
            ERROR_AND_DIE(ss.str().c_str());
        }
        if(auto xml_root = doc.RootElement()) {
            DataUtils::ValidateXmlElement(*xml_root, "tileDefinitions", "spritesheet,tileDefinition", "");
            if(auto xml_spritesheet = xml_root->FirstChildElement("spritesheet")) {
                if(auto* spritesheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet)) {
                    DataUtils::ForEachChildElement(*xml_root, "tileDefinition",
                        [spritesheet](const XMLElement& elem) {
                        TileDefinition::CreateTileDefinition(elem, spritesheet);
                    });
                }
            }
        }
    }

    if(auto xml_layers = elem.FirstChildElement("layers")) {
        DataUtils::ValidateXmlElement(*xml_layers, "layers", "layer", "");
        std::size_t layer_count = DataUtils::GetChildElementCount(*xml_layers, "layer");
        int layer_index = 0;
        _layers.reserve(layer_count);
        DataUtils::ForEachChildElement(*xml_layers, "layer",
            [this, &layer_index](const XMLElement& xml_layer) {
            _layers.emplace_back(std::make_unique<Layer>(this, xml_layer));
            _layers.back()->z_index = layer_index++;            
        });
    }

    return true;
}
