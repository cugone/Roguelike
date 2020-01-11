#include "Game/Map.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/IntVector3.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/Actor.hpp"
#include "Game/Entity.hpp"
#include "Game/EntityText.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Layer.hpp"
#include "Game/TileDefinition.hpp"

#include <algorithm>
#include <sstream>

void Map::CreateTextEntity(const TextEntityDesc& desc) noexcept {
    const auto text = EntityText::CreateTextEntity(desc);
    _entities.push_back(text);
}

void Map::CreateTextEntityAt(const IntVector2& tileCoords, const TextEntityDesc& desc) noexcept {
    auto desc_copy = desc;
    desc_copy.position = _renderer.ConvertWorldToScreenCoords(Vector2(tileCoords) + Vector2(0.5f, 0.5f));
    const auto text = EntityText::CreateTextEntity(desc_copy);
    _entities.push_back(text);
}


void Map::ShakeCamera(const IntVector2& from, const IntVector2& to) noexcept {
    const auto distance = MathUtils::CalculateManhattanDistance(from, to);
    camera.trauma += 0.1f + distance * 0.05f;
}

void Map::SetDebugGridColor(const Rgba& gridColor) {
    auto* layer = GetLayer(0);
    layer->debug_grid_color = gridColor;
}

void Map::KillEntity(Entity& e) {
    e.tile->entity = nullptr;
}


std::vector<Entity*> Map::GetEntities() const noexcept {
    return _entities;
}

AABB2 Map::CalcWorldBounds() const {
    return {Vector2::ZERO, CalcMaxDimensions()};
}

std::vector<Tile*> Map::PickTilesFromWorldCoords(const Vector2& worldCoords) const {
    auto world_bounds = CalcWorldBounds();
    if(MathUtils::IsPointInside(world_bounds, worldCoords)) {
        std::vector<Tile*> results = GetTiles(IntVector2{ worldCoords });
        return results;
    }
    return {};
}

Tile* Map::PickTileFromWorldCoords(const Vector2& worldCoords, int layerIndex) const {
    auto world_bounds = CalcWorldBounds();
    if(MathUtils::IsPointInside(world_bounds, worldCoords)) {
        return GetTile(IntVector3{ worldCoords, layerIndex });
    }
    return nullptr;
}

std::vector<Tile*> Map::PickTilesFromMouseCoords(const Vector2& mouseCoords) const {
    const auto& world_coords = _renderer.ConvertScreenToWorldCoords(camera, mouseCoords);
    return PickTilesFromWorldCoords(world_coords);
}

Vector2 Map::WorldCoordsToScreenCoords(const Vector2& worldCoords) const {
    return _renderer.ConvertWorldToScreenCoords(worldCoords);
}

Vector2 Map::ScreenCoordsToWorldCoords(const Vector2& screenCoords) const {
    return _renderer.ConvertScreenToWorldCoords(camera, screenCoords);
}

Tile* Map::PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const {
    const auto& world_coords = _renderer.ConvertScreenToWorldCoords(camera, mouseCoords);
    return PickTileFromWorldCoords(world_coords, layerIndex);
}

bool Map::MoveOrAttack(Actor* actor, Tile* tile) {
    if(!actor || !tile) {
        return false;
    }
    if(actor->MoveTo(tile)) {
        return true;
    } else {
        if(!tile->entity) {
            return false;
        }
        const auto dmg_result = Entity::Fight(*actor, *tile->entity);
        TextEntityDesc desc{};
        desc.position = Vector2(tile->GetCoords()) + Vector2{ 0.5f, 0.5f };
        desc.font = g_theGame->ingamefont;
        desc.color = Rgba::White;
        if(dmg_result >= 0) {
            desc.text = std::to_string(dmg_result);
            CreateTextEntityAt(tile->GetCoords(), desc);
        } else {
            desc.text = "MISS";
            CreateTextEntityAt(tile->GetCoords(), desc);
        }
        return true;
    }
}

Map::Map(Renderer& renderer, const XMLElement& elem) noexcept
    : _renderer(renderer)
{
    if(!LoadFromXML(elem)) {
        ERROR_AND_DIE("Could not load map.");
    }
}

Map::~Map() noexcept {
    _entities.clear();
    _entities.shrink_to_fit();
}

void Map::BeginFrame() {
    for(auto& layer : _layers) {
        layer->BeginFrame();
    }
}

void Map::Update(TimeUtils::FPSeconds deltaSeconds) {
    UpdateLayers(deltaSeconds);
    UpdateEntities(deltaSeconds);
}

void Map::UpdateLayers(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& layer : _layers) {
        layer->Update(deltaSeconds);
    }
}

void Map::SetPriorityLayer(std::size_t i) {
    if(i >= _layers.size()) {
        return;
    }
    BringLayerToFront(i);
}

void Map::BringLayerToFront(std::size_t i) {
    auto first = std::begin(_layers);
    auto curr = first + i;
    auto next = curr + 1;
    auto end = std::end(_layers);
    while(next != end) {
        std::iter_swap(curr, next);
        curr++;
        next = curr + 1;
    }
}

void Map::UpdateEntities(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& entity : _entities) {
        entity->Update(deltaSeconds);
    }
}

void Map::UpdateEntityAI(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& entity : _entities) {
        entity->UpdateAI(deltaSeconds);
    }
}

void Map::Render(Renderer& renderer) const {
    for(const auto& layer : _layers) {
        layer->Render(renderer);
    }

    if(auto* tile = PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
        auto tile_bounds = tile->GetBounds();
        renderer.SetMaterial(renderer.GetMaterial("__2D"));
        renderer.SetModelMatrix(Matrix4::I);
        renderer.DrawAABB2(tile_bounds, Rgba::White, Rgba::NoAlpha, Vector2::ONE * 0.0625f);
    }

    static std::vector<Vertex3D> vbo{};
    vbo.clear();
    static std::vector<unsigned int> ibo{};
    ibo.clear();

    g_theRenderer->ResetModelViewProjection();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, camera.GetAspectRatio());
    g_theRenderer->SetCamera(camera);

    for(const auto* e : _entities) {
        const auto* eAsText = dynamic_cast<const EntityText*>(e);
        if(eAsText) {
            eAsText->Render(vbo, ibo, Rgba::White, 0);
        }
    }

}

void Map::DebugRender(Renderer& renderer) const {
    for(const auto& layer : _layers) {
        layer->DebugRender(renderer);
    }
    if(!g_theGame->_debug_render) {
        return;
    }
    if(g_theGame->_show_grid) {
        renderer.SetModelMatrix(Matrix4::I);
        const auto* layer = GetLayer(0);
        renderer.DrawWorldGrid2D(layer->tileDimensions, layer->debug_grid_color);
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
    _entities.erase(std::remove_if(std::begin(_entities), std::end(_entities), [](const auto& e)->bool { return !e || (e && e->GetStats().GetStat(StatsID::Health) <= 0); }), std::end(_entities));
}

bool Map::IsTileInView(const IntVector2& tileCoords) {
    return IsTileInView(IntVector3{ tileCoords, 0 });
}

bool Map::IsTileInView(const IntVector3& tileCoords) {
    return IsTileInView(GetTile(tileCoords));
}

bool Map::IsTileInView(Tile* tile) {
    if(!tile || (tile && !tile->layer)) {
        return false;
    }
    const auto tile_bounds = tile->GetBounds();
    const auto view_bounds = tile->layer->CalcViewBounds(Vector2(tile->GetCoords()));
    return MathUtils::DoAABBsOverlap(tile_bounds, view_bounds);
}

bool Map::IsEntityInView(Entity* entity) {
    return entity && IsTileInView(entity->tile);
}

void Map::FocusTileAt(const IntVector3& position) {
    if(const auto* tile = GetTile(position)) {
        camera.SetPosition(Vector3{ position });
    }
}

void Map::FocusEntity(Entity* entity) {
    if(entity) {
        FocusTileAt(IntVector3(entity->tile->GetCoords(), entity->layer->z_index));
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
        [](const std::unique_ptr<Layer>& a, const std::unique_ptr<Layer>& b)->bool {
        return a->viewHeight < b->viewHeight;
    });
    return (*max_view_height_elem)->viewHeight;
}

Material* Map::GetTileMaterial() const {
    return _current_tileMaterial;
}

void Map::SetTileMaterial(Material* material) {
    _current_tileMaterial = material;
}

void Map::ResetTileMaterial() {
    _current_tileMaterial = _default_tileMaterial;
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

std::vector<Tile*> Map::GetTiles(const IntVector2& location) const {
    return GetTiles(location.x, location.y);
}

Tile* Map::GetTile(const IntVector3& locationAndLayerIndex) const {
    return GetTile(locationAndLayerIndex.x, locationAndLayerIndex.y, locationAndLayerIndex.z);
}

//TODO: std::optional return
std::vector<Tile*> Map::GetTiles(int x, int y) const {
    std::vector<Tile*> results{};
    for(auto i = std::size_t{ 0 }; i < GetLayerCount(); ++i) {
        if(auto* cur_layer = GetLayer(i)) {
            results.push_back(cur_layer->GetTile(x, y));
        }
    }
    if(std::all_of(std::begin(results), std::end(results), [](const Tile* t) { return t == nullptr; })) {
        results.clear();
        results.shrink_to_fit();
    }
    return results;
}

Tile* Map::GetTile(int x, int y, int z) const {
    if(auto* layer = GetLayer(z)) {
        return layer->GetTile(x, y);
    }
    return nullptr;
}

bool Map::LoadFromXML(const XMLElement& elem) {

    DataUtils::ValidateXmlElement(elem, "map", "tiles,layers,material", "name", "actors,features");

    LoadNameForMap(elem);
    LoadMaterialsForMap(elem);
    LoadTileDefinitionsForMap(elem);
    LoadLayersForMap(elem);
    LoadItemsForMap(elem);
    LoadActorsForMap(elem);
    LoadFeaturesForMap(elem);
    return true;
}

void Map::LoadNameForMap(const XMLElement& elem) {
    std::ostringstream ss;
    ss << "MAP " << ++default_map_index;
    std::string default_name = ss.str();
    _name = DataUtils::ParseXmlAttribute(elem, "name", default_name);
}

void Map::LoadMaterialsForMap(const XMLElement& elem) {
    if(auto xml_material = elem.FirstChildElement("material")) {
        DataUtils::ValidateXmlElement(*xml_material, "material", "", "name");
        auto src = DataUtils::ParseXmlAttribute(*xml_material, "name", std::string{ "__invalid" });
        _default_tileMaterial = g_theRenderer->GetMaterial(src);
        _current_tileMaterial = _default_tileMaterial;
    }
}

void Map::LoadLayersForMap(const XMLElement &elem) {
    if(auto xml_layers = elem.FirstChildElement("layers")) {
        DataUtils::ValidateXmlElement(*xml_layers, "layers", "layer", "");
        std::size_t layer_count = DataUtils::GetChildElementCount(*xml_layers, "layer");
        if(layer_count > max_layers) {
            std::ostringstream ss;
            ss << "Layer count of map " << _name << " is greater than the maximum allowed (" << max_layers << ").";
            ss << "\nOnly the first " << max_layers << " layers will be used.";
            ss.flush();
            g_theFileLogger->LogLine(ss.str());
        }

        auto layer_index = 0;
        _layers.reserve(layer_count);
        DataUtils::ForEachChildElement(*xml_layers, "layer",
            [this, &layer_index](const XMLElement& xml_layer) {
            if(static_cast<std::size_t>(layer_index) < max_layers) {
                _layers.emplace_back(std::make_unique<Layer>(this, xml_layer));
                _layers.back()->z_index = layer_index++;
            }
        });
        _layers.shrink_to_fit();
    }
}

void Map::LoadTileDefinitionsForMap(const XMLElement& elem) {
    if(auto xml_tileset = elem.FirstChildElement("tiles")) {
        DataUtils::ValidateXmlElement(*xml_tileset, "tiles", "", "src");
        const auto src = DataUtils::ParseXmlAttribute(*xml_tileset, "src", std::string{});
        if(src.empty()) {
            ERROR_AND_DIE("Map tiles source is empty.");
        }
        LoadTileDefinitionsFromFile(src);
    }
}

void Map::LoadTileDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        std::ostringstream ss;
        ss << "Entities file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Map " << _name << " failed to load. Tiles source file at " << src << " could not be loaded.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    if(auto xml_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_root, "tileDefinitions", "spritesheet,tileDefinition", "");
        if(auto xml_spritesheet = xml_root->FirstChildElement("spritesheet")) {
            _tileset_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet);
            if(_tileset_sheet) {
                DataUtils::ForEachChildElement(*xml_root, "tileDefinition",
                    [this](const XMLElement& elem) {
                    TileDefinition::CreateTileDefinition(*g_theRenderer, elem, _tileset_sheet);
                });
            }
        }
    }
}

void Map::LoadActorsForMap(const XMLElement& elem) {
    if(auto* xml_actors = elem.FirstChildElement("actors")) {
        DataUtils::ValidateXmlElement(*xml_actors, "actors", "actor", "");
        const auto actor_count = DataUtils::GetChildElementCount(*xml_actors, "actor");
        DataUtils::ForEachChildElement(*xml_actors, "actor",
            [this](const XMLElement& elem) {
            auto* actor = Actor::CreateActor(this, elem);
            auto actor_name = StringUtils::ToLowerCase(actor->name);
            bool is_player = actor_name == "player";
            if(player && is_player) {
                ERROR_AND_DIE("Map failed to load. Multiplayer not yet supported.");
            }
            if(is_player) {
                player = actor;
                player->OnMove.Subscribe_method(this, &Map::ShakeCamera);
            }
            _entities.push_back(actor);
        });
    }
}

void Map::LoadFeaturesForMap(const XMLElement& elem) {
    if(auto* xml_features = elem.FirstChildElement("features")) {
        DataUtils::ValidateXmlElement(*xml_features, "features", "feature", "");
        const auto actor_count = DataUtils::GetChildElementCount(*xml_features, "feature");
        DataUtils::ForEachChildElement(*xml_features, "feature",
            [this](const XMLElement& elem) {
            auto* feature = Actor::CreateActor(this, elem);
            auto feature_name = StringUtils::ToLowerCase(feature->name);
            _entities.push_back(feature);
        });
    }
}

void Map::LoadItemsForMap(const XMLElement& elem) {
    if(auto* xml_map_equipment_source = elem.FirstChildElement("items")) {
        DataUtils::ValidateXmlElement(*xml_map_equipment_source, "items", "", "src");
        const auto src = DataUtils::ParseXmlAttribute(*xml_map_equipment_source, "src", std::string{});
        if(src.empty()) {
            ERROR_AND_DIE("Map item source is empty. Do not define element if map does not have items.");
        }
    }
}
