#include "Game/Map.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/IntVector3.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/Actor.hpp"
#include "Game/Cursor.hpp"
#include "Game/Entity.hpp"
#include "Game/Feature.hpp"
#include "Game/EntityText.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Layer.hpp"
#include "Game/Inventory.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Tile.hpp"

#include <algorithm>
#include <sstream>

void Map::CreateTextEntity(const TextEntityDesc& desc) noexcept {
    const auto text = EntityText::CreateTextEntity(desc);
    _text_entities.push_back(text);
}

void Map::CreateTextEntityAt(const IntVector2& tileCoords, TextEntityDesc desc) noexcept {
    const auto text_width = 1.0f / desc.font->CalculateTextWidth(desc.text);
    const auto text_height = 1.0f / desc.font->CalculateTextHeight(desc.text);
    const auto text_half_width = text_width * 0.5f;
    const auto text_half_height = text_height * 0.5f;
    const auto text_center_offset = Vector2{text_half_width, text_half_height};
    const auto tile_center = Vector2(tileCoords) + Vector2{0.5f, 0.5f};
    desc.position = _renderer.ConvertWorldToScreenCoords(camera, tile_center - text_center_offset);
    CreateTextEntity(desc);
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
    if(auto* asActor = dynamic_cast<Actor*>(&e)) {
        KillActor(*asActor);
    } else if(auto* asFeature = dynamic_cast<Feature*>(&e)) {
        KillFeature(*asFeature);
    }
}

void Map::KillActor(Actor& a) {
    a.tile->actor = nullptr;
}

void Map::KillFeature(Feature& f) {
    f.tile->feature = nullptr;
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
        return GetTiles(IntVector2{ worldCoords });
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
    return _renderer.ConvertWorldToScreenCoords(camera, worldCoords);
}

Vector2 Map::ScreenCoordsToWorldCoords(const Vector2& screenCoords) const {
    return _renderer.ConvertScreenToWorldCoords(camera, screenCoords);
}

IntVector2 Map::TileCoordsFromWorldCoords(const Vector2& worldCoords) const {
    return IntVector2{worldCoords};
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
        if(!tile->actor && !tile->feature) {
            return false;
        }
        if(tile->actor) {
            Entity::Fight(*actor, *tile->actor);
        } else if(tile->feature) {
            Entity::Fight(*actor, *tile->feature);
        }
        actor->Act();
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
    for(auto& actor : _actors) {
        actor->Act(false);
    }
    for(auto& layer : _layers) {
        layer->BeginFrame();
    }
}

void Map::Update(TimeUtils::FPSeconds deltaSeconds) {
    UpdateLayers(deltaSeconds);
    UpdateTextEntities(deltaSeconds);
    UpdateEntities(deltaSeconds);
}

void Map::UpdateLayers(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& layer : _layers) {
        layer->Update(deltaSeconds);
    }
}

void Map::UpdateEntities(TimeUtils::FPSeconds deltaSeconds) {
    UpdateActorAI(deltaSeconds);
}

void Map::UpdateTextEntities(TimeUtils::FPSeconds deltaSeconds) {
    for(auto* entity : _text_entities) {
        entity->Update(deltaSeconds);
    }
}

void Map::UpdateActorAI(TimeUtils::FPSeconds /*deltaSeconds*/) {
    for(auto& actor : _actors) {
        const auto is_player = actor == player;
        const auto player_acted = player->Acted();
        const auto is_alive = actor->GetStats().GetStat(StatsID::Health) > 0;
        const auto is_visible = actor->tile->canSee;
        const auto should_update = !is_player && player_acted && is_alive;
        if(should_update) {
            if(auto* behavior = actor->GetCurrentBehavior()) {
                behavior->Act(actor);
            }
        }
    }
}

void Map::SetPriorityLayer(std::size_t i) {
    if(i >= _layers.size()) {
        return;
    }
    BringLayerToFront(i);
}

void Map::BringLayerToFront(std::size_t i) {
    /*
    * This is essentially bubble sort but, for less than 10 elements, I don't care.
    */
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

void Map::Render(Renderer& renderer) const {
    for(const auto& layer : _layers) {
        layer->Render(renderer);
    }

    static std::vector<Vertex3D> verts;
    verts.clear();
    static std::vector<unsigned int> ibo;
    ibo.clear();

    auto& ui_camera = g_theGame->ui_camera;

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{-ui_view_half_extents.x, ui_view_half_extents.y};
    auto ui_rightTop = Vector2{ui_view_half_extents.x, -ui_view_half_extents.y};
    auto ui_nearFar = Vector2{0.0f, 1.0f};
    ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, ui_camera.GetAspectRatio());
    g_theRenderer->SetCamera(ui_camera);


    for(auto* entity : _text_entities) {
        entity->Render(verts, ibo, entity->color, 0);
    }

    g_theRenderer->SetCamera(camera);

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
        renderer.SetMaterial(renderer.GetMaterial("__2D"));
        renderer.DrawAABB2(bounds, Rgba::Cyan, Rgba::NoAlpha);
    }
    if(g_theGame->_show_raycasts) {
        const auto* map = g_theGame->_map.get();
        const auto* p = map->player;
        const auto vision_range = p->visibility;
        const auto& startTile = *map->GetTile(IntVector3(p->GetPosition(), 0));
        const auto tiles = map->GetTilesWithinDistance(startTile, vision_range);
        renderer.SetModelMatrix(Matrix4::I);
        renderer.SetMaterial(renderer.GetMaterial("__2D"));
        const auto start = player->tile->GetBounds().CalcCenter();
        for(const auto& tile : tiles) {
            const auto end = tile->GetBounds().CalcCenter();
            const auto resultVisible = map->Raycast(start, end, true, [map](const IntVector2& tileCoords) { return map->IsTileOpaque(tileCoords); });
            const auto resultPassable = map->Raycast(start, end, true, [map](const IntVector2& tileCoords) { return map->IsTilePassable(tileCoords); });
            renderer.DrawLine2D(start, end, Rgba::White);
            if(resultVisible.didImpact) {
                const auto normalStart = resultVisible.impactPosition;
                const auto normalEnd = resultVisible.impactPosition + resultVisible.impactSurfaceNormal * 0.5f;
                renderer.DrawLine2D(normalStart, normalEnd, Rgba::Red);
            }
            if(resultPassable.didImpact) {
                for(const auto& impactedImpassableTileCoords : resultPassable.impactTileCoords) {
                    auto* cur_tile = map->GetTile(IntVector3{impactedImpassableTileCoords, 0});
                    if(!cur_tile->IsPassable()) {
                        cur_tile->color = Rgba::Red;
                    }
                }
            }
        }
    }
}

void Map::EndFrame() {
    for(auto& layer : _layers) {
        layer->EndFrame();
    }
    //for(auto* entity : _text_entities) {
    //    entity->EndFrame();
    //}
    _entities.erase(std::remove_if(std::begin(_entities), std::end(_entities), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_entities));
    _text_entities.erase(std::remove_if(std::begin(_text_entities), std::end(_text_entities), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_text_entities));
    _actors.erase(std::remove_if(std::begin(_actors), std::end(_actors), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_actors));
    _features.erase(std::remove_if(std::begin(_features), std::end(_features), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_features));
}

bool Map::IsTileInView(const IntVector2& tileCoords) const {
    return IsTileInView(IntVector3{ tileCoords, 0 });
}

bool Map::IsTileInView(const IntVector3& tileCoords) const {
    return IsTileInView(GetTile(tileCoords));
}

bool Map::IsTileInView(Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    const auto tile_bounds = tile->GetBounds();
    const auto view_bounds = tile->layer->CalcViewBounds(camera.position);
    return MathUtils::DoAABBsOverlap(tile_bounds, view_bounds);
}

bool Map::IsEntityInView(Entity* entity) const {
    return entity && IsTileInView(entity->tile);
}

bool Map::IsTileSolid(const IntVector2& tileCoords) const {
    return IsTileSolid(IntVector3{ tileCoords, 0});
}


bool Map::IsTileSolid(const IntVector3& tileCoords) const {
    return IsTileSolid(GetTile(tileCoords));
}

bool Map::IsTileSolid(Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    return tile->IsSolid();
}

bool Map::IsTileOpaque(const IntVector2& tileCoords) const {
    return IsTileOpaque(IntVector3{tileCoords, 0});
}

bool Map::IsTileOpaque(const IntVector3& tileCoords) const {
    return IsTileOpaque(GetTile(tileCoords));
}

bool Map::IsTileOpaque(Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    return tile->IsOpaque();
}

bool Map::IsTileOpaqueOrSolid(const IntVector2& tileCoords) const {
    return IsTileOpaqueOrSolid(IntVector3{tileCoords, 0});
}

bool Map::IsTileOpaqueOrSolid(const IntVector3& tileCoords) const {
    return IsTileOpaqueOrSolid(GetTile(tileCoords));
}

bool Map::IsTileOpaqueOrSolid(Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    return tile->IsOpaque() || tile->IsSolid();
}

bool Map::IsTileVisible(const IntVector2& tileCoords) const {
    return IsTileVisible(IntVector3{tileCoords, 0});
}

bool Map::IsTileVisible(const IntVector3& tileCoords) const {
    return IsTileVisible(GetTile(tileCoords));
}

bool Map::IsTileVisible(Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    return tile->IsVisible();
}

bool Map::IsTilePassable(const IntVector2& tileCoords) const {
    return IsTilePassable(IntVector3{tileCoords, 0});
}

bool Map::IsTilePassable(const IntVector3& tileCoords) const {
    return IsTilePassable(GetTile(tileCoords));
}

bool Map::IsTilePassable(Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    return tile->IsPassable();
}

void Map::FocusTileAt(const IntVector3& position) {
    if(GetTile(position)) {
        camera.SetPosition(Vector3{ position });
    }
}

void Map::FocusEntity(Entity* entity) {
    if(entity) {
        FocusTileAt(IntVector3(entity->tile->GetCoords(), entity->layer->z_index));
    }
}

bool Map::HasLineOfSight(const Vector2& startPosition, const Vector2& endPosition) const {
    const auto displacement = endPosition - startPosition;
    const auto direction = displacement.GetNormalize();
    float length = displacement.CalcLength();
    return HasLineOfSight(startPosition, direction, length);
}

bool Map::HasLineOfSight(const Vector2& startPosition, const Vector2& direction, float maxDistance) const {
    RaycastResult2D result = Raycast(startPosition, direction, maxDistance, true, [this](const IntVector2& tileCoords)->bool { return this->IsTileOpaque(tileCoords); });
    return !result.didImpact;
}

bool Map::IsTileWithinDistance(const Tile& startTile, unsigned int manhattanDist) const {
    auto visibleTiles = GetTilesWithinDistance(startTile, manhattanDist);
    bool isWithinDistance = false;
    for(auto& t : visibleTiles) {
        auto calculatedManhattanDist = MathUtils::CalculateManhattanDistance(startTile.GetCoords(), t->GetCoords());
        if(calculatedManhattanDist < manhattanDist) {
            isWithinDistance = true;
            break;
        }
    }
    return isWithinDistance;
}

bool Map::IsTileWithinDistance(const Tile& startTile, float dist) const {
    auto visibleTiles = GetTilesWithinDistance(startTile, dist);
    bool isWithinDistance = false;
    for(auto& t : visibleTiles) {
        auto calculatedDistSq = (Vector2(startTile.GetCoords()) - Vector2(t->GetCoords())).CalcLengthSquared();
        if(calculatedDistSq < dist * dist) {
            isWithinDistance = true;
            break;
        }
    }
    return isWithinDistance;
}

std::vector<Tile*> Map::GetTilesWithinDistance(const Tile& startTile, unsigned int manhattanDist) const {
    std::vector<Tile*> results{};
    const auto& layer0 = _layers[0];
    for(auto& tile : *layer0) {
        auto calculatedManhattanDist = MathUtils::CalculateManhattanDistance(startTile.GetCoords(), tile.GetCoords());
        if(calculatedManhattanDist < manhattanDist) {
            results.push_back(&tile);
        }
    }
    return results;
}

std::vector<Tile*> Map::GetTilesWithinDistance(const Tile& startTile, float dist) const {
    std::vector<Tile*> results;
    const auto& layer0 = _layers[0];
    for(auto& tile : *layer0) {
        auto calculatedDistSq = (Vector2(startTile.GetCoords()) - Vector2(tile.GetCoords())).CalcLengthSquared();
        if(calculatedDistSq < dist * dist) {
            results.push_back(&tile);
        }
    }
    return results;
}

std::vector<Tile*> Map::GetVisibleTilesWithinDistance(const Tile& startTile, unsigned int manhattanDist) const {
    std::vector<Tile*> results = GetTilesWithinDistance(startTile, manhattanDist);
    results.erase(std::remove_if(std::begin(results), std::end(results), [this, &startTile](Tile* tile) { return !HasLineOfSight(Vector2(startTile.GetCoords()) + Vector2{0.5f, 0.5f}, Vector2(tile->GetCoords()) + Vector2{0.5f, 0.5f}); }), std::end(results));
    return results;
}

std::vector<Tile*> Map::GetVisibleTilesWithinDistance(const Tile& startTile, float dist) const {
    std::vector<Tile*> results = GetTilesWithinDistance(startTile, dist);
    results.erase(std::remove_if(std::begin(results), std::end(results), [this, &startTile](Tile* tile) { return !HasLineOfSight(Vector2(startTile.GetCoords()) + Vector2{0.5f, 0.5f}, Vector2(tile->GetCoords()) + Vector2{0.5f, 0.5f}); }), std::end(results));
    return results;
}

Map::RaycastResult2D Map::StepAndSample(const Vector2& startPosition, const Vector2& endPosition, float sampleRate) const {
    const auto displacement = endPosition - startPosition;
    const auto direction = displacement.GetNormalize();
    float length = displacement.CalcLength();
    return StepAndSample(startPosition, direction, length, sampleRate);
}


Map::RaycastResult2D Map::StepAndSample(const Vector2& startPosition, const Vector2& direction, float maxDistance, float sampleRate) const {
    auto endPosition = startPosition + (direction * maxDistance);
    const auto stepFrequency = 1.0f / sampleRate;
    const auto stepRate = direction * stepFrequency;
    auto currentSamplePoint = startPosition;
    IntVector2 currentTileCoords{startPosition};
    IntVector2 endTileCoords{endPosition};
    RaycastResult2D result;
    if(IsTileSolid(currentTileCoords)) {
        result.didImpact = true;
        result.impactFraction = 0.0f;
        result.impactPosition = currentSamplePoint;
        result.impactTileCoords.insert(currentTileCoords);
        result.impactSurfaceNormal = -direction;
        return result;
    }
    
    while(true) {
        result.impactTileCoords.insert(currentTileCoords);
        currentSamplePoint += stepRate;
        Vector2 EP = currentSamplePoint - endPosition;
        currentTileCoords = TileCoordsFromWorldCoords(currentSamplePoint);
        if(MathUtils::DotProduct(direction, EP) > 0.0f) {
            result.didImpact = false;
            result.impactFraction = 1.0f;
            result.impactTileCoords.insert(currentTileCoords);
            return result;
        }
        Vector2 SP = currentSamplePoint - startPosition;
        if(MathUtils::DotProduct(direction, SP) < 0.0f) {
            result.didImpact = false;
            result.impactFraction = 0.0f;
            result.impactTileCoords.insert(currentTileCoords);
            return result;
        }
        if(IsTileSolid(currentTileCoords)) {
            result.didImpact = true;
            result.impactFraction = currentSamplePoint.CalcLength() / maxDistance;
            result.impactPosition = currentSamplePoint;
            result.impactTileCoords.insert(currentTileCoords);
            result.impactSurfaceNormal = -direction;
            return result;
        }
    }
}

Vector2 Map::CalcMaxDimensions() const {
    Vector2 results{ 1.0f, 1.0f };
    std::for_each(std::begin(_layers), std::end(_layers), [&results](const auto& layer) {
        const auto& cur_layer_dimensions = layer->tileDimensions;
        if(results.x < cur_layer_dimensions.x) {
            results.x = static_cast<float>(cur_layer_dimensions.x);
        }
        if(results.y < cur_layer_dimensions.y) {
            results.y = static_cast<float>(cur_layer_dimensions.y);
        }
    });
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

    DataUtils::ValidateXmlElement(elem, "map", "tiles,layers,material", "name", "actors,features,items");

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
    const auto default_name = std::string{"MAP "} + std::to_string(++default_map_index);
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
            const auto ss = std::string{"Layer count of map "} + _name + " is greater than the amximum allowed (" + std::to_string(max_layers) +")."
                            "\nOnly the first " + std::to_string(max_layers) + " layers will be used.";
            g_theFileLogger->LogLine(ss);
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
        const auto ss = std::string{"Entities file at "} +src.string() + " could not be found.";
        ERROR_AND_DIE(ss.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        const auto ss = std::string("Map ") + _name + " failed to load. Tiles source file at " + src.string() + " could not be loaded.";
        ERROR_AND_DIE(ss.c_str());
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
        DataUtils::ForEachChildElement(*xml_actors, "actor",
            [this](const XMLElement& elem) {
            auto* actor = Actor::CreateActor(this, elem);
            auto actor_name = StringUtils::ToLowerCase(actor->name);
            bool is_player = actor_name == "player";
            if(player && is_player) {
                ERROR_AND_DIE("Map failed to load. Multiplayer not yet supported.");
            }
            actor->SetFaction(Faction::Enemy);
            if(is_player) {
                player = actor;
                //player->OnMove.Subscribe_method(this, &Map::ShakeCamera);
                player->SetFaction(Faction::Player);
            }
            _entities.push_back(actor);
            _actors.push_back(actor);
        });
    }
}

void Map::LoadFeaturesForMap(const XMLElement& elem) {
    if(auto* xml_features = elem.FirstChildElement("features")) {
        DataUtils::ValidateXmlElement(*xml_features, "features", "feature", "");
        DataUtils::ForEachChildElement(*xml_features, "feature",
            [this](const XMLElement& elem) {
            auto* feature = Feature::CreateFeature(this, elem);
            _entities.push_back(feature);
            _features.push_back(feature);
        });
    }
}

void Map::LoadItemsForMap(const XMLElement& elem) {
    if(auto* xml_items = elem.FirstChildElement("items")) {
        DataUtils::ValidateXmlElement(*xml_items, "items", "item", "");
        DataUtils::ForEachChildElement(*xml_items, "item", [this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "item", "", "name,position");
            const auto name = DataUtils::ParseXmlAttribute(elem, "name", nullptr);
            const auto pos = DataUtils::ParseXmlAttribute(elem, "position", IntVector2{-1, -1});
            if(auto* tile = this->GetTile(IntVector3(pos, 0))) {
                tile->inventory.AddItem(Item::GetItem(name));
            } else {
                //TODO Add StringUtils::to_string(const IntVector2/3/4&);
                std::ostringstream ss;
                ss << "Invalid tile " << pos << " for item \"" << name << "\" placement.";
                g_theFileLogger->LogLineAndFlush(ss.str());
            }
        });
    }
}
