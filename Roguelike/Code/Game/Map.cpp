#include "Game/Map.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Math/Vector4.hpp"
#include "Engine/Math/IntVector3.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Renderer/Material.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/Adventure.hpp"
#include "Game/Actor.hpp"
#include "Game/Cursor.hpp"
#include "Game/Entity.hpp"
#include "Game/EntityText.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Feature.hpp"
#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Inventory.hpp"
#include "Game/Layer.hpp"
#include "Game/MapGenerator.hpp"
#include "Game/Pathfinder.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Tile.hpp"

#include <algorithm>
#include <sstream>

const Rgba& Map::GetSkyColorForDay() noexcept {
    static const Rgba g_clearcolor_day = Rgba::SkyBlue;
    return g_clearcolor_day;
}

const Rgba& Map::GetSkyColorForNight() noexcept {
    static const Rgba g_clearcolor_night = Rgba::MidnightBlue;
    return g_clearcolor_night;
}

const Rgba& Map::GetSkyColorForCave() noexcept {
    static const Rgba g_clearcolor_cave = Rgba::Black;
    return g_clearcolor_cave;
}

void Map::SetCursorForFaction(const Actor* actor) const noexcept {
    switch(actor->GetFaction()) {
    case Faction::None:
        g_theGame->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
        break;
    case Faction::Player:
        g_theGame->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
        break;
    case Faction::Enemy:
        g_theGame->SetCurrentCursorById(CursorId::Red_Crosshair_Box);
        break;
    case Faction::Neutral:
        g_theGame->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
        break;
    default:
        g_theGame->SetCurrentCursorById(CursorId::Green_Box);
        break;
    }
}

void Map::SetCursorForTile() const noexcept {
    g_theGame->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
    if(auto* tile = this->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile != nullptr) {
        if(!tile->CanSee()) {
            g_theGame->SetCurrentCursorById(CursorId::Question);
        } else if(tile->actor) {
            SetCursorForFaction(tile->actor);
        }
    }
}

void Map::ShouldRenderStatWindow() noexcept {
    _should_render_stat_window = false;
    if(auto* tile = this->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile && tile->actor) {
        _should_render_stat_window = true;
    }
}

bool Map::AllowLightingDuringDay() const noexcept {
    if(_current_global_light == day_light_value) {
        return _allow_lighting_calculations_during_day;
    }
    return true; //Not Day, lighting is a good idea.
}

void Map::CalculateLightingForLayers([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    for(auto& layer : _layers) {
        CalculateLighting(layer.get());
    }
}

void Map::CreateTextEntity(const TextEntityDesc& desc) noexcept {
    const auto text = EntityText::CreateTextEntity(desc);
    text->map = this;
    text->layer = this->GetLayer(0);
    _text_entities.push_back(text);
}

void Map::CreateTextEntityAt(const IntVector2& tileCoords, TextEntityDesc desc) noexcept {
    const auto tile_center = Vector2(tileCoords) + Vector2{0.5f, 0.5f};
    desc.position = tile_center;
    CreateTextEntity(desc);
}


void Map::ShakeCamera(const IntVector2& from, const IntVector2& to) noexcept {
    const auto distance = MathUtils::CalculateManhattanDistance(from, to);
    cameraController.GetCamera().trauma += 0.1f + distance * 0.05f;
}

void Map::SetGlobalLightFromSkyColor() noexcept {
    if(_current_sky_color == GetSkyColorForDay()) {
        _current_global_light = day_light_value;
    } else if(_current_sky_color == GetSkyColorForNight()) {
        _current_global_light = night_light_value;
    } else if(_current_sky_color == GetSkyColorForCave()) {
        _current_global_light = min_light_value;
    }
}

Rgba Map::SkyColor() const noexcept {
    return _current_sky_color;
}

void Map::SetSkyColorToDay() noexcept {
    _current_sky_color = GetSkyColorForDay();
}

void Map::SetSkyColorToNight() noexcept {
    _current_sky_color = GetSkyColorForNight();
}

void Map::SetSkyColorToCave() noexcept {
    _current_sky_color = GetSkyColorForCave();
}

std::vector<Tile*> Map::GetViewableTiles() const noexcept {
    std::vector<Tile*> result{};
    for(auto& layer : _layers) {
        const auto tiles = GetTilesInArea(layer->CalcCullBounds(cameraController.GetCamera().GetPosition()));
        result.reserve(result.size() + tiles.size());
        for(const auto& tile : tiles) {
            if(tile) {
                result.push_back(tile);
            }
        }
    }
    return result;
}

std::vector<Tile*> Map::GetTilesInArea(const AABB2& bounds) const {
    const auto dims = bounds.CalcDimensions();
    const auto width = static_cast<std::size_t>(dims.x);
    const auto height = static_cast<std::size_t>(dims.y);
    std::vector<Tile*> results;
    results.reserve(width * height);
    for(int x = static_cast<int>(bounds.mins.x); x <= bounds.maxs.x; ++x) {
        for(int y = static_cast<int>(bounds.mins.y); y <= bounds.maxs.y; ++y) {
            results.push_back(GetTile(x, y, 0));
        }
    }
    return results;
}

std::size_t Map::DebugTilesInViewCount() const {
    _debug_tiles_in_view_count = 0;
    for(const auto& layer : _layers) {
        _debug_tiles_in_view_count += layer->debug_tiles_in_view_count;
    }
    return _debug_tiles_in_view_count;
}

std::size_t Map::DebugVisibleTilesInViewCount() const {
    _debug_visible_tiles_in_view_count = 0;
    for(const auto& layer : _layers) {
        _debug_visible_tiles_in_view_count += layer->debug_visible_tiles_in_view_count;
    }
    return _debug_visible_tiles_in_view_count;
}

void Map::RegenerateMap() noexcept {
    _map_generator->Generate();
}

Pathfinder* Map::GetPathfinder() const noexcept {
    return _pathfinder.get();
}

void Map::ZoomOut() noexcept {
    cameraController.ZoomOut();
    for(auto& layer : _layers) {
        layer->DirtyMesh();
    }
}

void Map::ZoomIn() noexcept {
    cameraController.ZoomIn();
    for(auto& layer : _layers) {
        layer->DirtyMesh();
    }
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

const std::vector<Entity*>& Map::GetEntities() const noexcept {
    return _entities;
}

const std::vector<EntityText*>& Map::GetTextEntities() const noexcept {
    return _text_entities;
}

AABB2 Map::CalcWorldBounds() const {
    return {Vector2::ZERO, CalcMaxDimensions()};
}

AABB2 Map::CalcCameraBounds() const {
    auto bounds = CalcWorldBounds();
    const auto cam_dims = cameraController.GetCamera().GetViewDimensions();
    const auto cam_w = cam_dims.x * 0.5f;
    const auto cam_h = cam_dims.y * 0.5f;
    bounds.AddPaddingToSides(-cam_w, -cam_h);
    return bounds;
}

std::optional<std::vector<Tile*>> Map::PickTilesFromWorldCoords(const Vector2& worldCoords) const {
    auto world_bounds = CalcWorldBounds();
    if(MathUtils::IsPointInside(world_bounds, worldCoords)) {
        return GetTiles(IntVector2{worldCoords});
    }
    return {};
}

Tile* Map::PickTileFromWorldCoords(const Vector2& worldCoords, int layerIndex) const {
    auto world_bounds = CalcWorldBounds();
    if(MathUtils::IsPointInside(world_bounds, worldCoords)) {
        return GetTile(IntVector3{worldCoords, layerIndex});
    }
    return nullptr;
}

std::optional<std::vector<Tile*>> Map::PickTilesFromMouseCoords(const Vector2& mouseCoords) const {
    const auto& world_coords = ServiceLocator::get<IRendererService>().ConvertScreenToWorldCoords(cameraController.GetCamera(), mouseCoords);
    return PickTilesFromWorldCoords(world_coords);
}

Vector2 Map::WorldCoordsToScreenCoords(const Vector2& worldCoords) const {
    return ServiceLocator::get<IRendererService>().ConvertWorldToScreenCoords(cameraController.GetCamera(), worldCoords);
}

Vector2 Map::ScreenCoordsToWorldCoords(const Vector2& screenCoords) const {
    return ServiceLocator::get<IRendererService>().ConvertScreenToWorldCoords(cameraController.GetCamera(), screenCoords);
}

IntVector2 Map::TileCoordsFromWorldCoords(const Vector2& worldCoords) const {
    return IntVector2{worldCoords};
}

Tile* Map::PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const {
    const auto& world_coords = ServiceLocator::get<IRendererService>().ConvertScreenToWorldCoords(cameraController.GetCamera(), mouseCoords);
    return PickTileFromWorldCoords(world_coords, layerIndex);
}

Vector2 Map::GetSubTileLocationFromMouseCoords(const Vector2& mouseCoords) const noexcept {
    const auto& world_coords = ServiceLocator::get<IRendererService>().ConvertScreenToWorldCoords(cameraController.GetCamera(), mouseCoords);
    const auto&& [x_int, x_frac] = MathUtils::SplitFloatingPointValue(world_coords.x);
    const auto&& [y_int, y_frac] = MathUtils::SplitFloatingPointValue(world_coords.y);
    return Vector2{x_frac, y_frac};
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

Map::Map(const XMLElement& elem) noexcept
    : _root_xml_element(elem)
    , _pathfinder(std::make_unique<Pathfinder>())
{
    GUARANTEE_OR_DIE(LoadFromXML(elem), "Could not load map.");
    cameraController = OrthographicCameraController{};
    cameraController.SetZoomLevelRange(Vector2{8.0f, 16.0f});
    for(auto& layer : _layers) {
        InitializeLighting(layer.get());
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
    cameraController.Update(deltaSeconds);
    UpdateLayers(deltaSeconds);
    UpdateTextEntities(deltaSeconds);
    UpdateEntities(deltaSeconds);
    if(AllowLightingDuringDay()) {
        CalculateLightingForLayers(deltaSeconds);
        UpdateLighting(deltaSeconds);
    }
    FocusCameraOnPlayer(deltaSeconds);
    ShouldRenderStatWindow();
    SetCursorForTile();
}

void Map::UpdateLayers(TimeUtils::FPSeconds deltaSeconds) {
    for(auto& layer : _layers) {
        layer->Update(deltaSeconds);
    }
    UpdateCursor(deltaSeconds);
    AddCursorToTopLayer();
}

void Map::FocusCameraOnPlayer(TimeUtils::FPSeconds deltaSeconds) noexcept {
    cameraController.TranslateTo(Vector2{player->tile->GetCoords()} + Vector2{0.5f, 0.5f}, deltaSeconds);
    const auto clamped_camera_position = MathUtils::CalcClosestPoint(cameraController.GetCamera().GetPosition(), CalcCameraBounds());
    cameraController.SetPosition(clamped_camera_position);
}

void Map::UpdateCursor(TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(const auto& tiles = PickTilesFromMouseCoords(g_theInputSystem->GetMouseCoords()); tiles.has_value()) {
        if(g_theGame->current_cursor) {
            g_theGame->current_cursor->SetCoords((*tiles).back()->GetCoords());
            g_theGame->current_cursor->Update(deltaSeconds);
        }
    }
}

void Map::AddCursorToTopLayer() noexcept {
    if(g_theGame->current_cursor) {
        if(auto* layer = GetLayer(GetLayerCount() - std::size_t{1u}); layer) {
            layer->AppendToMesh(g_theGame->current_cursor);
        }
    }
}

void Map::UpdateEntities(TimeUtils::FPSeconds deltaSeconds) {
    UpdateActorAI(deltaSeconds);
    for(auto* actor : _actors) {
        actor->CalculateLightValue();
    }
    for(auto* feature : _features) {
        feature->CalculateLightValue();
    }
}

void Map::UpdateTextEntities(TimeUtils::FPSeconds deltaSeconds) {
    for(auto* entity : _text_entities) {
        entity->Update(deltaSeconds);
    }
}

void Map::UpdateLighting(TimeUtils::FPSeconds /*deltaSeconds*/) noexcept {
    while(!_lightingQueue.empty()) {
        auto& ti = _lightingQueue.front();
        _lightingQueue.pop();
        ti.ClearLightDirty();
        UpdateTileLighting(ti);
        ti.layer->DirtyMesh();
    }
}

void Map::InitializeLighting(Layer* layer) noexcept {
    if(layer == nullptr) {
        return;
    }
    const auto tileCount = static_cast<std::size_t>(layer->tileDimensions.x) * static_cast<std::size_t>(layer->tileDimensions.y);
    for(auto i = std::size_t{0u}; i != tileCount; ++i) {
        TileInfo ti{layer, i};
        ti.SetLightValue(_current_global_light);
    }
    layer->DirtyMesh();
}

void Map::CalculateLighting(Layer* layer) noexcept {
    if(layer == nullptr) {
        return;
    }
    const auto width = static_cast<std::size_t>(layer->tileDimensions.x);
    const auto height = static_cast<std::size_t>(layer->tileDimensions.y);
    const auto tileCount = width * height;
    for(auto i = std::size_t{}; i != tileCount; ++i) {
        TileInfo ti{layer, i};
        if(ti.IsOpaque()) {
            std::array<uint32_t, 8> lightvalues{};
            auto iter = std::begin(lightvalues);
            for(auto& n : ti.GetAllNeighbors()) {
                if(!n.IsOpaque() && n.index != ti.index) {
                    (*iter) = n.GetLightValue();
                }
                ++iter;
            }
            if(auto max_light_neighbor_value = *(std::max_element)(std::cbegin(lightvalues), std::cend(lightvalues))) {
                ti.SetLightValue(max_light_neighbor_value - uint32_t{1u});
                ti.SetLightDirty();
            }
        } else {
            DirtyValidNeighbors(ti);
        }
    }
}

void Map::DirtyTileLight(TileInfo& ti) noexcept {
    if(!ti.IsLightDirty()) {
        ti.SetLightDirty();
        _lightingQueue.push(ti);
    }
}

void Map::UpdateTileLighting(TileInfo& ti) noexcept {
    const uint32_t idealLighting = [&]() { // Ideal Lighting of tile is highest available from self, neighbor, sky, actor, or feature...
        const auto self_value = ti.GetSelfIlluminationValue();
        const auto highestNeighborLightValue = [&]() {
            if(auto value = ti.GetMaxLightValueFromNeighbors(); value) {
                return value - uint32_t{1u};
            }
            return uint32_t{0u};
        }(); //IIIL
        const auto actor_value = ti.GetActorLightValue();
        const auto feature_value = ti.GetFeatureLightValue();
        const auto sky_value = [&]() { //If sky, light value is global value
            if(ti.IsSky()) {
                return _current_global_light;
            }
            return uint32_t{0u};
        }(); //IIIL
        const auto edge_value = [&]() { //If at edge, "edge" is global value, so actual tile is one less (if able) or zero.
            if(auto value = _current_global_light; ti.IsAtEdge() && value) {
                return value - uint32_t{1u};
            }
            return uint32_t{0u};
        }(); //IIIL
        return (std::max)({edge_value, sky_value, self_value, highestNeighborLightValue, actor_value, feature_value});
    }(); //IIIL
    if(idealLighting != ti.GetLightValue()) {
        ti.SetLightValue(idealLighting);
        ti.SetLightDirty();
        if(ti.IsOpaque()) {
            DirtyNeighborLighting(ti, Layer::NeighborDirection::North);
            DirtyNeighborLighting(ti, Layer::NeighborDirection::East);
            DirtyNeighborLighting(ti, Layer::NeighborDirection::South);
            DirtyNeighborLighting(ti, Layer::NeighborDirection::West);
        }
    }
}

void Map::DirtyValidNeighbors(TileInfo& ti) noexcept {
    auto n = ti.GetNorthNeighbor();
    if(!n.IsSky() && !n.IsOpaque()) {
        DirtyTileLight(n);
    }
    auto e = ti.GetEastNeighbor();
    if(!e.IsSky() && !e.IsOpaque()) {
        DirtyTileLight(e);
    }
    auto s = ti.GetSouthNeighbor();
    if(!s.IsSky() && !s.IsOpaque()) {
        DirtyTileLight(s);
    }
    auto w = ti.GetWestNeighbor();
    if(!w.IsSky() && !w.IsOpaque()) {
        DirtyTileLight(w);
    }
}

void Map::DirtyCardinalNeighbors(TileInfo& ti) noexcept {
    auto n = ti.GetNorthNeighbor();
    DirtyTileLight(n);
    auto e = ti.GetEastNeighbor();
    DirtyTileLight(e);
    auto s = ti.GetSouthNeighbor();
    DirtyTileLight(s);
    auto w = ti.GetWestNeighbor();
    DirtyTileLight(w);
}


void Map::DirtyNeighborLighting(TileInfo& ti, const Layer::NeighborDirection& direction) noexcept {
    TileInfo neighbor = [&]() {
        switch(direction) {
        case Layer::NeighborDirection::North:
            return ti.GetNorthNeighbor();
        case Layer::NeighborDirection::East:
            return ti.GetEastNeighbor();
        case Layer::NeighborDirection::South:
            return ti.GetSouthNeighbor();
        case Layer::NeighborDirection::West:
            return ti.GetWestNeighbor();
        default:
            ERROR_AND_DIE("Map::DirtyNeighborLighting: Invalid neighbor direction.");
        }
    }(); //IIIL
    DirtyTileLight(neighbor);
}

void Map::UpdateActorAI(TimeUtils::FPSeconds /*deltaSeconds*/) {
    for(auto& actor : _actors) {
        const auto is_player = actor == player;
        const auto player_acted = player->Acted();
        const auto is_alive = actor->GetStats().GetStat(StatsID::Health) > 0;
        const auto should_update = !is_player && player_acted && is_alive;
        if(should_update) {
            if(auto* behavior = actor->GetCurrentBehavior()) {
                behavior->Act(actor);
            }
        }
    }
}

void Map::RenderStatsBlock(Actor* actor) const noexcept {
    if(!actor) {
        return;
    }
    const auto& stats = actor->GetStats();
    g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));

    const auto text = [&]()->std::string {
        std::ostringstream ss{};
        ss << "Lvl: " << stats.GetStat(StatsID::Level) << '\n';
        ss << "HP: " << stats.GetStat(StatsID::Health) << '\n';
        ss << "Max HP: " << stats.GetStat(StatsID::Health_Max) << '\n';
        ss << "XP: " << stats.GetStat(StatsID::Experience) << '\n';
        ss << "Atk: " << stats.GetStat(StatsID::Attack) << '\n';
        ss << "Def: " << stats.GetStat(StatsID::Defense) << '\n';
        ss << "Spd: " << stats.GetStat(StatsID::Speed) << '\n';
        ss << "Eva: " << stats.GetStat(StatsID::Evasion) << '\n';
        ss << "Lck: " << stats.GetStat(StatsID::Luck);
        return ss.str();
    }();
    const auto text_height = g_theGame->ingamefont->CalculateTextHeight(text);
    const auto text_width = g_theGame->ingamefont->CalculateTextWidth(text);
    AABB2 bounds{};
    const auto dims_w = text_width;
    const auto dims_h = text_height;
    const auto bottom_right = Vector2{dims_w, dims_h};
    const auto element_padding = Vector2{2.0f, -5.0f};
    const auto margin_padding = Vector2{0.0f, 0.0f};
    const auto border_padding = Vector2{2.0f, 2.0f};
    const auto padding = element_padding + margin_padding + border_padding;
    bounds.StretchToIncludePoint(bottom_right);
    bounds.Translate(Vector2{50.0f, 50.0f});
    const auto text_position = bounds.mins + padding;
    g_theRenderer->DrawAABB2(bounds, actor->GetFactionAsColor(), Rgba{50, 50, 50, 128}, border_padding);
    auto S = Matrix4::I;
    auto R = Matrix4::I;
    auto T = Matrix4::CreateTranslationMatrix(text_position);
    auto M = Matrix4::MakeSRT(S, R, T);
    g_theRenderer->SetModelMatrix(M);
    g_theRenderer->DrawMultilineText(g_theGame->ingamefont, text);
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
    for(auto& layer : _layers) {
        layer->DirtyMesh();
    }
}

void Map::Render() const {
    for(const auto& layer : _layers) {
        layer->Render();
    }

    auto& ui_camera = g_theGame->ui_camera;

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;

    g_theRenderer->BeginHUDRender(ui_camera, ui_view_half_extents, ui_view_height);

    for(auto* entity : _text_entities) {
        entity->Render();
    }

    if(_should_render_stat_window) {
        if(const auto* tile = this->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile != nullptr) {
            RenderStatsBlock(tile->actor);
        }
    }

}

void Map::DebugRender() const {
#ifdef UI_DEBUG
    for(const auto& layer : _layers) {
        layer->DebugRender();
    }
    if(!g_theGame->_debug_render) {
        return;
    }
    if(g_theGame->_debug_show_grid) {
        g_theRenderer->SetModelMatrix(Matrix4::I);
        const auto* layer = GetLayer(0);
        g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        g_theRenderer->DrawWorldGrid2D(layer->tileDimensions, layer->debug_grid_color);
    }
    if(g_theGame->_debug_show_room_bounds) {
        if(auto* generator = dynamic_cast<RoomsMapGenerator*>(_map_generator.get()); generator != nullptr) {
            for(auto& room : generator->rooms) {
                g_theRenderer->SetModelMatrix(Matrix4::I);
                g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
                g_theRenderer->DrawAABB2(room, Rgba::Cyan, Rgba::NoAlpha);
            }
        }
    }
    if(g_theGame->_debug_show_world_bounds) {
        auto bounds = CalcWorldBounds();
        g_theRenderer->SetModelMatrix(Matrix4::I);
        g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        g_theRenderer->DrawAABB2(bounds, Rgba::Cyan, Rgba::NoAlpha);
    }
    if(g_theGame->_debug_show_camera_bounds) {
        auto bounds = CalcCameraBounds();
        g_theRenderer->SetModelMatrix(Matrix4::I);
        g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        g_theRenderer->DrawAABB2(bounds, Rgba::Orange, Rgba::NoAlpha);
    }
    if(g_theGame->_debug_show_camera) {
        const auto& cam_pos = cameraController.GetCamera().GetPosition();
        g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        g_theRenderer->DrawCircle2D(cam_pos, 0.5f, Rgba::Cyan);
        g_theRenderer->DrawAABB2(GetLayer(0)->CalcViewBounds(cam_pos), Rgba::Green, Rgba::NoAlpha);
        g_theRenderer->DrawAABB2(GetLayer(0)->CalcCullBounds(cam_pos), Rgba::Blue, Rgba::NoAlpha);
    }
#endif
}

void Map::EndFrame() {
    for(auto& layer : _layers) {
        layer->EndFrame();
    }
    for(auto& e : _text_entities) {
        e->EndFrame();
    }

    _entities.erase(std::remove_if(std::begin(_entities), std::end(_entities), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_entities));
    _text_entities.erase(std::remove_if(std::begin(_text_entities), std::end(_text_entities), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_text_entities));
    _actors.erase(std::remove_if(std::begin(_actors), std::end(_actors), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_actors));
    _features.erase(std::remove_if(std::begin(_features), std::end(_features), [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(_features));

}

bool Map::IsPlayerOnExit() const noexcept {
    return player->tile->IsExit();
}

bool Map::IsPlayerOnEntrance() const noexcept {
    return player->tile->IsEntrance();
}

void Map::SetParentAdventure(Adventure* parent) noexcept {
    _parent_adventure = parent;
}

bool Map::IsTileInArea(const AABB2& bounds, const IntVector2& tileCoords) const {
    return IsTileInArea(bounds, IntVector3{tileCoords, 0});
}

bool Map::IsTileInArea(const AABB2& bounds, const IntVector3& tileCoords) const {
    return IsTileInArea(bounds, GetTile(tileCoords));
}

bool Map::IsTileInArea(const AABB2& bounds, const Tile* tile) const {
    if(!tile) {
        return false;
    }
    return MathUtils::DoAABBsOverlap(bounds, tile->GetBounds());
}

bool Map::IsTileInView(const IntVector2& tileCoords) const {
    return IsTileInView(IntVector3{tileCoords, 0});
}

bool Map::IsTileInView(const IntVector3& tileCoords) const {
    return IsTileInView(GetTile(tileCoords));
}

bool Map::IsTileInView(const Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    const auto tile_bounds = tile->GetBounds();
    const auto view_bounds = tile->layer->CalcCullBounds(cameraController.GetCamera().position);
    return MathUtils::DoAABBsOverlap(tile_bounds, view_bounds);
}

bool Map::IsEntityInView(Entity* entity) const {
    return entity && IsTileInView(entity->tile);
}

bool Map::IsTileSolid(const IntVector2& tileCoords) const {
    return IsTileSolid(IntVector3{tileCoords, 0});
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

bool Map::IsTileVisible(const Tile* tile) const {
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

bool Map::IsTilePassable(const Tile* tile) const {
    if(!tile || !tile->layer) {
        return false;
    }
    return tile->IsPassable();
}

bool Map::IsTileEntrance(const IntVector2& tileCoords) const {
    return IsTileEntrance(IntVector3{tileCoords, 0});
}

bool Map::IsTileEntrance(const IntVector3& tileCoords) const {
    return IsTileEntrance(GetTile(tileCoords));
}

bool Map::IsTileEntrance(const Tile* tile) const {
    if(!tile || tile->layer) {
        return false;
    }
    return tile->IsEntrance();
}

bool Map::IsTileExit(const IntVector2& tileCoords) const {
    return IsTileExit(IntVector3{tileCoords, 0});
}

bool Map::IsTileExit(const IntVector3& tileCoords) const {
    return IsTileExit(GetTile(tileCoords));
}

bool Map::IsTileExit(const Tile* tile) const {
    if(!tile || tile->layer) {
        return false;
    }
    return tile->IsExit();
}

void Map::FocusTileAt(const IntVector3& position) {
    if(GetTile(position)) {
        cameraController.SetPosition(Vector2{IntVector2{position.x, position.y}});
    }
}

void Map::FocusEntity(const Entity* entity) {
    if(entity) {
        FocusTileAt(IntVector3(entity->tile->GetCoords(), entity->layer->z_index));
        g_theGame->current_cursor->SetCoords(entity->tile->GetCoords());
    }
}

Map::RaycastResult2D Map::HasLineOfSight(const Vector2& startPosition, const Vector2& endPosition) const {
    const auto displacement = endPosition - startPosition;
    const auto direction = displacement.GetNormalize();
    float length = displacement.CalcLength();
    return HasLineOfSight(startPosition, direction, length);
}

Map::RaycastResult2D Map::HasLineOfSight(const Vector2& startPosition, const Vector2& direction, float maxDistance) const {
    return Raycast(startPosition, direction, maxDistance, true, [this](const IntVector2& tileCoords)->bool { return this->IsTileOpaque(tileCoords); });
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
    return this->GetTilesWithinDistance(startTile, static_cast<float>(manhattanDist), [&](const IntVector2& start, const IntVector2& end) { return MathUtils::CalculateManhattanDistance(start, end); });
    //std::vector<Tile*> results{};
    //const auto& layer0 = _layers[0];
    //for(auto& tile : *layer0) {
    //    auto calculatedManhattanDist = MathUtils::CalculateManhattanDistance(startTile.GetCoords(), tile.GetCoords());
    //    if(calculatedManhattanDist < manhattanDist) {
    //        results.push_back(&tile);
    //    }
    //}
    //return results;
}

std::vector<Tile*> Map::GetTilesWithinDistance(const Tile& startTile, float dist) const {
    return this->GetTilesWithinDistance(startTile, dist * dist, [&](const IntVector2& start, const IntVector2& end) { return (Vector2(end) - Vector2(start)).CalcLengthSquared(); });
    //std::vector<Tile*> results;
    //const auto& layer0 = _layers[0];
    //for(auto& tile : *layer0) {
    //    auto calculatedDistSq = (Vector2(startTile.GetCoords()) - Vector2(tile.GetCoords())).CalcLengthSquared();
    //    if(calculatedDistSq < dist * dist) {
    //        results.push_back(&tile);
    //    }
    //}
    //return results;
}

std::vector<Tile*> Map::GetVisibleTilesWithinDistance(const Tile& startTile, unsigned int manhattanDist) const {
    std::vector<Tile*> results = GetTilesWithinDistance(startTile, manhattanDist);
    results.erase(std::remove_if(std::begin(results), std::end(results), [this, &startTile](Tile* tile) { return tile->IsInvisible(); }), std::end(results));
    results.shrink_to_fit();
    return results;
}

std::vector<Tile*> Map::GetVisibleTilesWithinDistance(const Tile& startTile, float dist) const {
    std::vector<Tile*> results = GetTilesWithinDistance(startTile, dist);
    results.erase(std::remove_if(std::begin(results), std::end(results), [this, &startTile](Tile* tile) { return tile->IsInvisible(); }), std::end(results));
    results.shrink_to_fit();
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
    Vector2 currentSamplePoint = startPosition;
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
    Vector2 results{1.0f, 1.0f};
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

std::optional<std::vector<Tile*>> Map::GetTiles(const IntVector2& location) const {
    return GetTiles(location.x, location.y);
}

Tile* Map::GetTile(const IntVector3& locationAndLayerIndex) const {
    return GetTile(locationAndLayerIndex.x, locationAndLayerIndex.y, locationAndLayerIndex.z);
}

std::optional<std::vector<Tile*>> Map::GetTiles(int x, int y) const {
    std::vector<Tile*> results{};
    for(auto i = std::size_t{0}; i < GetLayerCount(); ++i) {
        if(auto* cur_layer = GetLayer(i)) {
            results.push_back(cur_layer->GetTile(x, y));
        }
    }
    if(std::all_of(std::begin(results), std::end(results), [](const Tile* t) { return t == nullptr; })) {
        return {};
    }
    return std::make_optional(results);
}

Tile* Map::GetTile(int x, int y, int z) const {
    if(auto* layer = GetLayer(z)) {
        return layer->GetTile(x, y);
    }
    return nullptr;
}

bool Map::LoadFromXML(const XMLElement& elem) {

    DataUtils::ValidateXmlElement(elem, "map", "tiles,material,mapGenerator", "name", "actors,features,items", "timeOfDay,allowLightingDuringDay");
    LoadTimeOfDayForMap(elem);
    LoadNameForMap(elem);
    LoadMaterialsForMap(elem);
    LoadTileDefinitionsForMap(elem);
    GenerateMap(elem);
    return true;
}

void Map::GenerateMap(const XMLElement& elem) noexcept {
    LoadGenerator(elem);
}

void Map::LoadTimeOfDayForMap(const XMLElement& elem) {
    const auto value = StringUtils::ToLowerCase(DataUtils::ParseXmlAttribute(elem, "timeOfDay", std::string{"night"}));
    if(value == "day") {
        _current_sky_color = GetSkyColorForDay();
    } else if(value == "night") {
        _current_sky_color = GetSkyColorForNight();
    } else if(value == "cave") {
        _current_sky_color = GetSkyColorForCave();
    } else {
        DebuggerPrintf("Invalid timeOfDay value. Defaulting to day.\n");
        _current_sky_color = GetSkyColorForDay();
    }
    SetGlobalLightFromSkyColor();
    _allow_lighting_calculations_during_day = DataUtils::ParseXmlAttribute(elem, "allowLightingDuringDay", false);
}

void Map::LoadNameForMap(const XMLElement& elem) {
    const auto default_name = std::string{"MAP "} + std::to_string(++default_map_index);
    _name = DataUtils::ParseXmlAttribute(elem, "name", default_name);
}

void Map::LoadMaterialsForMap(const XMLElement& elem) {
    if(auto xml_material = elem.FirstChildElement("material")) {
        DataUtils::ValidateXmlElement(*xml_material, "material", "", "name");
        auto src = DataUtils::ParseXmlAttribute(*xml_material, "name", std::string{"__invalid"});
        _default_tileMaterial = g_theRenderer->GetMaterial(src);
        _current_tileMaterial = _default_tileMaterial;
    }
}

void Map::LoadGenerator(const XMLElement& elem) {
    const auto* xml_generator = elem.FirstChildElement("mapGenerator");
    CreateGeneratorFromTypename(*xml_generator);
}

void Map::CreateGeneratorFromTypename(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "mapGenerator", "", "", "", "type");
    const auto xml_type = DataUtils::ParseXmlAttribute(elem, "type", std::string{});
    if(xml_type == "heightmap") {
        _map_generator = std::make_unique<HeightMapGenerator>(this, elem);
        _map_generator->Generate();
    } else if(xml_type == "file") {
        _map_generator = std::make_unique<FileMapGenerator>(this, elem);
        _map_generator->Generate();
    } else if(xml_type == "maze") {
        MazeMapGenerator::Generate(this, elem);
    } else {
        _map_generator = std::make_unique<XmlMapGenerator>(this, elem);
        _map_generator->Generate();
    }
}

void Map::LoadTileDefinitionsForMap(const XMLElement& elem) {
    if(auto xml_tileset = elem.FirstChildElement("tiles")) {
        DataUtils::ValidateXmlElement(*xml_tileset, "tiles", "", "src");
        const auto src = DataUtils::ParseXmlAttribute(*xml_tileset, "src", std::string{});
        GUARANTEE_OR_DIE(!src.empty(), "Map tiles source is empty.");
        LoadTileDefinitionsFromFile(src);
    }
}

void Map::LoadTileDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    g_theGame->ThrowIfSourceFileNotFound(src);
    if(tinyxml2::XMLDocument doc{}; auto* xml_root = g_theGame->ThrowIfSourceFileNotLoaded(doc, src)) {
        DataUtils::ValidateXmlElement(*xml_root, "tileDefinitions", "spritesheet,tileDefinition", "");
        if(auto xml_spritesheet = xml_root->FirstChildElement("spritesheet")) {
            if(_tileset_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet); _tileset_sheet) {
                DataUtils::ForEachChildElement(*xml_root, "tileDefinition",
                    [this](const XMLElement& elem) {
                        auto* def = TileDefinition::CreateOrGetTileDefinition(elem, _tileset_sheet);
                        def->GetSprite()->SetMaterial(_current_tileMaterial);
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
                GUARANTEE_OR_DIE(!(player && is_player), "Map failed to load. Multiplayer not yet supported.");
                actor->SetFaction(Faction::Enemy);
                if(is_player) {
                    player = actor;
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
                if(auto* feature = Feature::CreateFeature(this, elem)) {
                    _entities.push_back(feature);
                    _features.push_back(feature);
                }
            });
    }
}

void Map::LoadItemsForMap(const XMLElement& elem) {
    if(auto* xml_items = elem.FirstChildElement("items")) {
        DataUtils::ValidateXmlElement(*xml_items, "items", "item", "");
        DataUtils::ForEachChildElement(*xml_items, "item", [this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "item", "", "name,position");
            const auto name = DataUtils::ParseXmlAttribute(elem, "name", std::string{});
            const auto pos = DataUtils::ParseXmlAttribute(elem, "position", IntVector2{-1, -1});
            if(auto* tile = this->GetTile(IntVector3(pos, 0))) {
                tile->AddItem(Item::GetItem(name));
            } else {
                const std::string error_msg{"Invalid tile " + StringUtils::to_string(pos) + " for item \"" + name + "\" placement."};
                g_theFileLogger->LogLineAndFlush(error_msg);
            }
            });
    }
}
