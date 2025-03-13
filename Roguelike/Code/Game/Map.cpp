#include "Game/Map.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/Base64.hpp"
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
#include "Game/Game.hpp"
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

void Map::SetCustomSkyColor(const Rgba& newColor) noexcept {
    _current_sky_color = newColor;
}

void Map::SetCursorForFaction(const Actor* actor) const noexcept {
    switch(actor->GetFaction()) {
    case Faction::None:
        GetGameAs<Game>()->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
        break;
    case Faction::Player:
        GetGameAs<Game>()->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
        break;
    case Faction::Enemy:
        GetGameAs<Game>()->SetCurrentCursorById(CursorId::Red_Crosshair_Box);
        break;
    case Faction::Neutral:
        GetGameAs<Game>()->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
        break;
    default:
        GetGameAs<Game>()->SetCurrentCursorById(CursorId::Green_Box);
        break;
    }
}

void Map::SetCursorForTile() const noexcept {
    GetGameAs<Game>()->SetCurrentCursorById(CursorId::Yellow_Corner_Box);
    if(auto* tile = this->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile != nullptr) {
        if(!tile->CanSee()) {
            GetGameAs<Game>()->SetCurrentCursorById(CursorId::Question);
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
    //if(_current_global_light == day_light_value) {
    //    return _allow_lighting_calculations_during_day;
    //}
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

void Map::SetSkyColorFromGlobalLight() noexcept {
    _current_sky_color = MathUtils::Interpolate(GetSkyColorForDay(), GetSkyColorForNight(), 1.0f - (static_cast<float>(_current_global_light) / static_cast<float>(max_light_value)));
}

void Map::DebugDisableLighting([[maybe_unused]] bool disableLighting) noexcept {
    if (_allow_lighting_calculations_during_day = disableLighting; _allow_lighting_calculations_during_day) {
        SetDebugGlobalLight(max_light_value);
        SetSkyColorFromGlobalLight();
        for (auto& layer : _layers) {
            InitializeLighting(layer.get());
        }
        CalculateLightingForLayers(TimeUtils::FPSeconds{ 0.0f });
        UpdateLighting(TimeUtils::FPSeconds{ 0.0f });
    }
}

void Map::DebugShowInvisibleTiles(bool show) noexcept {
    for(auto& layer : _layers) {
        layer->DebugShowInvisibleTiles(show);
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
    _map_generator.Generate();
}

const Pathfinder* Map::GetPathfinder() const noexcept {
    return &_pathfinder;
}

Pathfinder* Map::GetPathfinder() noexcept {
    return &_pathfinder;
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

void Map::SetDebugGlobalLight(uint32_t lightValue) {
    _current_global_light = lightValue;
}

int Map::GetCurrentGlobalLightValue() const noexcept {
    return _current_global_light;
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
    return {Vector2::Zero, CalcMaxDimensions()};
}

AABB2 Map::CalcCameraBounds() const {
    auto bounds = CalcWorldBounds();
    const auto cam_dims = cameraController.GetCamera().GetViewDimensions();
    const auto cam_w = cam_dims.x * 0.5f;
    const auto cam_h = cam_dims.y * 0.5f;
    bounds.AddPaddingToSides(-cam_w, -cam_h);
    return bounds;
}

std::size_t Map::ConvertLocationToIndex(const IntVector2& location) const noexcept {
    return ConvertLocationToIndex(location.x, location.y);
}

std::size_t Map::ConvertLocationToIndex(int x, int y) const noexcept {
    return x + static_cast<std::size_t>(y) * static_cast<int>(std::floor(CalcMaxDimensions().x));
}

IntVector2 Map::ConvertIndexToLocation(std::size_t index) const noexcept {
    const auto width = static_cast<int>(CalcMaxDimensions().x);
    const auto x = static_cast<int>(index % width);
    const auto y = static_cast<int>(index / width);
    return IntVector2{x, y};
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
    const auto& world_coords = ServiceLocator::get<IRendererService>()->ConvertScreenToWorldCoords(cameraController.GetCamera(), mouseCoords);
    return PickTilesFromWorldCoords(world_coords);
}

Vector2 Map::WorldCoordsToScreenCoords(const Vector2& worldCoords) const {
    return ServiceLocator::get<IRendererService>()->ConvertWorldToScreenCoords(cameraController.GetCamera(), worldCoords);
}

Vector2 Map::ScreenCoordsToWorldCoords(const Vector2& screenCoords) const {
    return ServiceLocator::get<IRendererService>()->ConvertScreenToWorldCoords(cameraController.GetCamera(), screenCoords);
}

IntVector2 Map::TileCoordsFromWorldCoords(const Vector2& worldCoords) const {
    return IntVector2{worldCoords};
}

Tile* Map::PickTileFromMouseCoords(const Vector2& mouseCoords, int layerIndex) const {
    const auto& world_coords = ServiceLocator::get<IRendererService>()->ConvertScreenToWorldCoords(cameraController.GetCamera(), mouseCoords);
    return PickTileFromWorldCoords(world_coords, layerIndex);
}

Vector2 Map::GetSubTileLocationFromMouseCoords(const Vector2& mouseCoords) const noexcept {
    auto world_coords = ServiceLocator::get<IRendererService>()->ConvertScreenToWorldCoords(cameraController.GetCamera(), mouseCoords);
    const auto [x_int, x_frac] = MathUtils::SplitFloatingPointValue(world_coords.x);
    const auto [y_int, y_frac] = MathUtils::SplitFloatingPointValue(world_coords.y);
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

Map::Map(const std::filesystem::path& filepath) noexcept
    : m_filepath{filepath}
{
    _xml_doc = std::make_shared<tinyxml2::XMLDocument>();
    if(const auto xml_success = _xml_doc->LoadFile(filepath.string().c_str()); xml_success == tinyxml2::XML_SUCCESS) {
        _root_xml_element = _xml_doc->RootElement();
    } else {
        ERROR_AND_DIE("Bad path for Map constructor");
    }
    Initialize(*_root_xml_element);
}

Map::Map(const XMLElement& elem) noexcept
    : _root_xml_element(const_cast<XMLElement*>(&elem))
{
    Initialize(*_root_xml_element);
}

Map::Map(IntVector2 dimensions) noexcept
{
    dimensions.x = std::clamp(dimensions.x, static_cast<int>(min_map_width), static_cast<int>(max_map_width));
    dimensions.y = std::clamp(dimensions.y, static_cast<int>(min_map_height), static_cast<int>(max_map_height));
    _layers.emplace_back(std::move(std::make_unique<Layer>(this, dimensions)));
    for(auto& layer : _layers) {
        InitializeLighting(layer.get());
    }
    _pathfinder.Initialize(dimensions);
    cameraController.SetZoomLevelRange(Vector2{ 8.0f, 16.0f });
}

Map::~Map() noexcept {
    _xml_doc.reset();
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
    CalculateLightingForLayers(deltaSeconds);
    UpdateLighting(deltaSeconds);
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
    if(player && player->tile) {
        cameraController.TranslateTo(Vector2{ player->tile->GetCoords() } + Vector2{ 0.5f, 0.5f }, deltaSeconds);
        const auto clamped_camera_position = MathUtils::CalcClosestPoint(cameraController.GetCamera().GetPosition(), CalcCameraBounds());
        cameraController.SetPosition(clamped_camera_position);
    }
}

void Map::UpdateCursor(TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(const auto& tiles = PickTilesFromMouseCoords(g_theInputSystem->GetMouseCoords()); tiles.has_value()) {
        if(GetGameAs<Game>()->current_cursor) {
            GetGameAs<Game>()->current_cursor->SetCoords((*tiles).back()->GetCoords());
            GetGameAs<Game>()->current_cursor->Update(deltaSeconds);
        }
    }
}

void Map::AddCursorToTopLayer() noexcept {
    if(GetGameAs<Game>()->current_cursor) {
        if(auto* layer = GetLayer(GetLayerCount() - std::size_t{1u}); layer) {
            layer->AppendToMesh(GetGameAs<Game>()->current_cursor);
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
        TileInfo& ti = _lightingQueue.front();
        _lightingQueue.pop_front();
        ti.ClearLightDirty();
        UpdateTileLighting(ti);
    }
}

void Map::InitializeLighting(Layer* layer) noexcept {
    if (layer == nullptr) {
        return;
    }
    layer->DirtyMesh();
    const auto tileCount = layer->tileDimensions.x * layer->tileDimensions.y;
    for (int i = 0; i < tileCount; ++i) {
        auto* currentTile = layer->GetTile(i);
        currentTile->SetLightValue(0);
        currentTile->SetLightDirty();
        //if (const auto* def = TileDefinition::GetTileDefinitionByName(currentTile->GetType()); def && def->self_illumination > 0) {
            TileInfo ti{};
            ti.index = i;
            ti.layer = layer;
            _lightingQueue.push_back(ti);
        //}
    }
    CalculateLighting(layer);
}

void Map::CalculateLighting(Layer* layer) noexcept {
    if(layer == nullptr) {
        return;
    }
    const auto width = static_cast<std::size_t>(layer->tileDimensions.x);
    const auto height = static_cast<std::size_t>(layer->tileDimensions.y);
    const auto tileCount = width * height;
    for (auto i = std::size_t{}; i != tileCount; ++i) {
        if (auto* tile = layer->GetTile(i); tile && tile->IsOpaque()) {
            break;
        } else {
            tile->SetSky();
            tile->SetLightValue(_current_global_light);
        }
    }
    for (auto i = std::size_t{}; i != tileCount; ++i) {
        TileInfo ti{ layer, i };
        if (ti.IsOpaque()) {
            break;
        }
        DirtyValidNeighbors(ti);
    }
}

void Map::DirtyValidNeighbors(TileInfo& ti) noexcept {
    if (auto n = ti.GetNorthNeighbor(); !n.IsSky() && !n.IsOpaque()) {
        DirtyTileLight(n);
    }
    if (auto e = ti.GetEastNeighbor(); !e.IsSky() && !e.IsOpaque()) {
        DirtyTileLight(e);
    }
    if (auto s = ti.GetSouthNeighbor(); !s.IsSky() && !s.IsOpaque()) {
        DirtyTileLight(s);
    }
    if (auto w = ti.GetWestNeighbor(); !w.IsSky() && !w.IsOpaque()) {
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

void Map::DirtyTileLight(TileInfo& ti) noexcept {
    if (ti.IsLightDirty()) {
        return;
    }
    _lightingQueue.push_back(ti);
    ti.SetLightDirty();
}

void Map::UpdateTileLighting(TileInfo& ti) noexcept {
    uint32_t idealLighting = ti.GetSelfIlluminationValue();
    if (!ti.IsOpaque()) {
        if (const auto highestNeighborLightValue = ti.GetMaxLightValueFromNeighbors(); highestNeighborLightValue > 0) {
            idealLighting = (std::max)(idealLighting, highestNeighborLightValue - 1);
        }
    }
    if (ti.IsSky() || (ti.IsAtEdge() && ti.IsOpaque())) {
        idealLighting = (std::max)(idealLighting, _current_global_light);
    }
    idealLighting = (std::max)(idealLighting, ti.GetActorLightValue());
    idealLighting = (std::max)(idealLighting, ti.GetFeatureLightValue());
    if (idealLighting != ti.GetLightValue()) {
        ti.SetLightValue(idealLighting);
        DirtyNeighborLighting(ti, Layer::NeighborDirection::North);
        DirtyNeighborLighting(ti, Layer::NeighborDirection::East);
        DirtyNeighborLighting(ti, Layer::NeighborDirection::South);
        DirtyNeighborLighting(ti, Layer::NeighborDirection::West);
    }
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
//clang-format off
    const auto text = std::format("Lvl: {}\nHP: {}\nMax HP: {}\nXP: {}\nAtk: {}\nDef: {}\nSpd: {}\nEva: {}\nLck: {}"
                                 ,stats.GetStat(StatsID::Level)
                                 ,stats.GetStat(StatsID::Health)
                                 ,stats.GetStat(StatsID::Health_Max)
                                 ,stats.GetStat(StatsID::Experience)
                                 ,stats.GetStat(StatsID::Attack)
                                 ,stats.GetStat(StatsID::Defense)
                                 ,stats.GetStat(StatsID::Speed)
                                 ,stats.GetStat(StatsID::Evasion)
                                 ,stats.GetStat(StatsID::Luck));
//clang-format on

    const auto text_height = GetGameAs<Game>()->ingamefont->CalculateTextHeight(text);
    const auto text_width = GetGameAs<Game>()->ingamefont->CalculateTextWidth(text);
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
    g_theRenderer->DrawAABB2(bounds, actor->GetFactionAsColor(), Rgba(50, 50, 50, 128), border_padding);
    const auto S = Matrix4::I;
    const auto R = Matrix4::I;
    const auto T = Matrix4::CreateTranslationMatrix(text_position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    g_theRenderer->SetModelMatrix(M);
    g_theRenderer->DrawMultilineText(GetGameAs<Game>()->ingamefont, text);
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

    auto& ui_camera = GetGameAs<Game>()->ui_camera;

    //2D View / HUD
    const auto ui_view_height = static_cast<float>(GetGameAs<Game>()->gameOptions.GetWindowHeight());
    const auto ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
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
    if(auto* game = GetGameAs<Game>(); game == nullptr) {
        return;
    } else {

        if(!game->_debug_render) {
            return;
        }
        if(game->_debug_show_grid) {
            g_theRenderer->SetModelMatrix(Matrix4::I);
            const auto* layer = GetLayer(0);
            g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
            g_theRenderer->DrawWorldGrid2D(layer->tileDimensions, layer->debug_grid_color);
        }
        if(game->_debug_show_room_bounds) {
            for(auto& room : _map_generator.rooms) {
                g_theRenderer->SetModelMatrix(Matrix4::I);
                g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
                g_theRenderer->DrawAABB2(room, Rgba::Cyan, Rgba::NoAlpha);
            }
        }
        if(game->_debug_show_world_bounds) {
            auto bounds = CalcWorldBounds();
            g_theRenderer->SetModelMatrix(Matrix4::I);
            g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
            g_theRenderer->DrawAABB2(bounds, Rgba::Cyan, Rgba::NoAlpha);
        }
        if(game->_debug_show_camera_bounds) {
            auto bounds = CalcCameraBounds();
            g_theRenderer->SetModelMatrix(Matrix4::I);
            g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
            g_theRenderer->DrawAABB2(bounds, Rgba::Orange, Rgba::NoAlpha);
        }
        if(game->_debug_show_camera) {
            const auto& cam_pos = cameraController.GetCamera().GetPosition();
            g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
            g_theRenderer->DrawCircle2D(cam_pos, 0.5f, Rgba::Cyan);
            g_theRenderer->DrawAABB2(GetLayer(0)->CalcViewBounds(cam_pos), Rgba::Green, Rgba::NoAlpha);
            g_theRenderer->DrawAABB2(GetLayer(0)->CalcCullBounds(cam_pos), Rgba::Blue, Rgba::NoAlpha);
        }
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
    std::erase_if(_entities, [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; });
    std::erase_if(_text_entities, [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; });
    std::erase_if(_actors, [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; });
    std::erase_if(_features, [](const auto& e)->bool { return !e || e->GetStats().GetStat(StatsID::Health) <= 0; });
}

bool Map::IsPlayerOnExit() const noexcept {
    return player->tile->IsExit();
}

bool Map::IsPlayerOnEntrance() const noexcept {
    return player->tile->IsEntrance();
}

void Map::Initialize(const XMLElement& elem) noexcept {
    GUARANTEE_OR_DIE(LoadFromXML(elem), "Could not load map.");
    cameraController = OrthographicCameraController{};
    cameraController.SetZoomLevelRange(Vector2{8.0f, 16.0f});
    for(auto& layer : _layers) {
        InitializeLighting(layer.get());
    }
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
        GetGameAs<Game>()->current_cursor->SetCoords(entity->tile->GetCoords());
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
    return results;
}

std::vector<Tile*> Map::GetVisibleTilesWithinDistance(const Tile& startTile, float dist) const {
    std::vector<Tile*> results = GetTilesWithinDistance(startTile, dist);
    results.erase(std::remove_if(std::begin(results), std::end(results), [this, &startTile](Tile* tile) { return tile->IsInvisible(); }), std::end(results));
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

std::optional<std::vector<Tile*>> Map::GetTiles(std::size_t index) const noexcept {
    return GetTiles(ConvertIndexToLocation(index));
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

bool Map::LoadFromTmx(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "map", "", "version,orientation,width,height,tilewidth,tileheight", "properties,editorsettings,tileset,layer,objectgroup,imagelayer,group", "tiledversion,class,renderorder,compressionlevel,parallaxoriginx,parallaxoriginy,backgroundcolor,nextlayerid,nextobjectid,infinite,hexsidelength,staggeraxis,staggerindex");

    {
        const auto verify_version = [](const XMLElement& elem, std::string versionAttributeName, const std::string requiredVersionString) {
            if(const auto version_string = DataUtils::ParseXmlAttribute(elem, versionAttributeName, std::string{ "0.0" }); version_string != requiredVersionString) {
                const auto required_versions = StringUtils::Split(requiredVersionString, '.', false);
                const auto actual_versions = StringUtils::Split(version_string, '.', false);
                const auto required_major_version = std::stoi(required_versions[0]);
                const auto required_minor_version = std::stoi(required_versions[1]);
                const auto actual_major_version = std::stoi(actual_versions[0]);
                const auto actual_minor_version = std::stoi(actual_versions[1]);
                if(actual_major_version < required_major_version || (actual_major_version == required_major_version && actual_minor_version < required_minor_version)) {
                    const auto msg = std::format("ERROR: Attribute mismatch for \"{}\". Required: {} File: {}\n", versionAttributeName, requiredVersionString, version_string);
                    ERROR_AND_DIE(msg.c_str());
                }
            }
        };


        constexpr auto required_tmx_version = "1.9";
        verify_version(elem, "version", required_tmx_version);

        constexpr auto required_tiled_version = "1.9.2";
        verify_version(elem, "tiledversion", required_tiled_version);
    }

    if(const auto prop_count = DataUtils::GetChildElementCount(elem, "properties"); prop_count > 1) {
        DebuggerPrintf(std::format("WARNING: TMX map file map element contains more than one \"properties\" element. Ignoring all after first.\n"));
    }
    if(const auto editorsettings_count = DataUtils::GetChildElementCount(elem, "editorsettings"); editorsettings_count > 1) {
        DebuggerPrintf(std::format("WARNING: TMX map file map element contains more than one \"editorsettings\" element. Ignoring all after first.\n"));
    }
    if(const auto* xml_editorsettings = elem.FirstChildElement("editorsettings"); xml_editorsettings != nullptr) {
        DataUtils::ValidateXmlElement(*xml_editorsettings, "editorsettings", "", "", "chunksize,export", "");
        if(const auto chunksize_count = DataUtils::GetChildElementCount(*xml_editorsettings, "chunksize"); chunksize_count > 1) {
            DebuggerPrintf(std::format("WARNING: TMX map file editorsettings element contains more than one \"chunksize\" element. Ignoring all after the first.\n"));
        }
        if(const auto export_count = DataUtils::GetChildElementCount(*xml_editorsettings, "export"); export_count > 1) {
            DebuggerPrintf(std::format("WARNING: TMX map file editorsettings element contains more than one \"export\" child element. Ignoring all after the first.\n"));
        }
        if(const auto* xml_chunksize = xml_editorsettings->FirstChildElement("chunksize"); xml_chunksize != nullptr) {
            DataUtils::ValidateXmlElement(*xml_chunksize, "chunksize", "", "", "", "width,height");
            m_chunkWidth = static_cast<uint16_t>(DataUtils::ParseXmlAttribute(*xml_chunksize, "width", 16));
            m_chunkHeight = static_cast<uint16_t>(DataUtils::ParseXmlAttribute(*xml_chunksize, "height", 16));
        }
        if(const auto* xml_export = xml_editorsettings->FirstChildElement("export"); xml_export != nullptr) {
            DataUtils::ValidateXmlElement(*xml_export, "export", "", "target,format");
            const auto target = DataUtils::GetAttributeAsString(*xml_export, "target");
            DebuggerPrintf(std::format("Map last exported as: {}.\n", target));
            const auto format = DataUtils::GetAttributeAsString(*xml_export, "format");
            DebuggerPrintf(std::format("Map last formatted as: {}.\n", format));
        }

    }

    const auto&& [firstgid, path] = ParseTmxTilesetElement(elem);
    if(DataUtils::HasChild(elem, "layer")) {
        ParseTmxTileLayerElements(elem, firstgid);
    }
    if(DataUtils::HasChild(elem, "objectgroup")) {

    }
    if(DataUtils::HasChild(elem, "imagelayer")) {

    }
    if(DataUtils::HasChild(elem, "group")) {

    }
    return true;
}

std::pair<int, std::filesystem::path> Map::ParseTmxTilesetElement(const XMLElement& elem) noexcept {
    if(!DataUtils::HasChild(elem, "tileset")) {
        DebuggerPrintf(std::format("TMX map load failure. Map {:s} is missing the element \"tileset\".\n", _name));
        return std::make_pair(0, std::filesystem::path{});
    }
    const auto* xml_tileset = elem.FirstChildElement("tileset");
    auto firstgid = DataUtils::ParseXmlAttribute(*xml_tileset, "firstgid", 1);
    if(!DataUtils::HasAttribute(*xml_tileset, "source")) {
        DebuggerPrintf(std::format("TMX map load failure. Map {:s} is missing the element \"source\".\n", _name));
        return std::make_pair(0, std::filesystem::path{});
        //DataUtils::ValidateXmlElement(*xml_tileset, "tileset", "", "name,tilewidth,tileheight,tilecount,columns", "image,tileoffset,grid,properties,terraintypes,wangsets,transformations", "version,tiledversion,class,spacing,margin,objectalignment,tilerendersize,fillmode");
        //LoadTmxTileset(*xml_tileset);
    } else {
        DataUtils::ValidateXmlElement(*xml_tileset, "tileset", "", "firstgid,source", "", "");
        const auto src = [this, xml_tileset]() {
            auto raw_src = std::filesystem::path{ DataUtils::ParseXmlAttribute(*xml_tileset, "source", std::string{}) };
            if(!raw_src.has_parent_path() || (raw_src.parent_path() != m_filepath.parent_path())) {
                raw_src = std::filesystem::canonical(m_filepath.parent_path() / raw_src);
            }
            raw_src.make_preferred();
            return raw_src;
        }();
        firstgid = DataUtils::ParseXmlAttribute(*xml_tileset, "firstgid", 1);
        return std::make_pair(firstgid, src);
        //if(const auto buffer = FileUtils::ReadStringBufferFromFile(src); buffer.has_value()) {
        //    tinyxml2::XMLDocument xml_tilesetDoc;
        //    if(const auto result = xml_tilesetDoc.Parse(buffer->data(), buffer->size()); result == tinyxml2::XML_SUCCESS) {
        //        const auto xml_root = xml_tilesetDoc.RootElement();
        //DataUtils::ValidateXmlElement(*xml_root, "tileset", "", "name,tilewidth,tileheight,tilecount,columns", "image,tileoffset,grid,properties,terraintypes,wangsets,transformations", "version,tiledversion,class,spacing,margin,objectalignment,tilerendersize,fillmode");
        //        LoadTmxTileset(*xml_root);
        //    }
        //}
        //return std::make_pair(0, std::filesystem::path{});
    }
}

bool Map::ParseTmxTileLayerElements(const XMLElement& elem, int firstgid) noexcept {
    const auto map_width = DataUtils::ParseXmlAttribute(elem, "width", min_map_width);
    const auto map_height = DataUtils::ParseXmlAttribute(elem, "height", min_map_height);
    if(const auto count = DataUtils::GetChildElementCount(elem, "layer"); count > 9) {
        g_theFileLogger->LogWarnLine(std::format("Layer count of TMX map {0} is greater than the maximum allowed ({1}).\nOnly the first {1} layers will be used.", _name, max_layers));
        g_theFileLogger->Flush();
    }
    DataUtils::ForEachChildElement(elem, "layer", [this, map_width, map_height, firstgid](const XMLElement& xml_layer) {
        DataUtils::ValidateXmlElement(xml_layer, "layer", "", "width,height", "properties,data", "id,name,class,x,y,opacity,visible,locked,tintcolor,offsetx,offsety,parallaxx,parallaxy");
        if(DataUtils::HasAttribute(xml_layer, "x") || DataUtils::HasAttribute(xml_layer, "y")) {
            g_theFileLogger->LogWarnLine(std::string{ "Attributes \"x\" and \"y\" in the layer element are deprecated and unsupported. Remove both attributes to suppress this message." });
            g_theFileLogger->Flush();
        }
        const auto layer_name = DataUtils::ParseXmlAttribute(xml_layer, "name", std::string{});
        if(DataUtils::HasChild(xml_layer, "properties")) {
            if(DataUtils::GetChildElementCount(xml_layer, "properties") > 1) {
                g_theFileLogger->LogWarnLine(std::format("WARNING: TMX map file layer element \"{}\" contains more than one \"properties\" element. Ignoring all after the first.\n", layer_name));
                g_theFileLogger->Flush();
            }
        }
        if(DataUtils::HasChild(xml_layer, "data")) {
            if(DataUtils::GetChildElementCount(xml_layer, "data") > 1) {
                g_theFileLogger->LogWarnLine(std::format("WARNING: TMX map file layer element \"{}\" contains more than one \"data\" element. Ignoring all after the first.\n", layer_name));
                g_theFileLogger->Flush();
            }
        }

        const auto layer_width = DataUtils::ParseXmlAttribute(xml_layer, "width", map_width);
        const auto layer_height = DataUtils::ParseXmlAttribute(xml_layer, "height", map_height);
        _layers.emplace_back(std::move(std::make_unique<Layer>(this, IntVector2{ layer_width, layer_height })));
        auto* layer = _layers.back().get();
        const auto clr_str = DataUtils::ParseXmlAttribute(xml_layer, "tintcolor", std::string{});
        layer->color.SetRGBAFromARGB(clr_str);
        layer->z_index = static_cast<int>(_layers.size()) - std::size_t{1u};
        if(DataUtils::HasChild(xml_layer, "data")) {
            auto* xml_data = xml_layer.FirstChildElement("data");
            InitializeTilesFromTmxData(layer, *xml_data, firstgid);
        }
    });
    return true;
}

void Map::InitializeTilesFromTmxData(Layer* layer, const XMLElement& elem, int firstgid) noexcept {
    DataUtils::ValidateXmlElement(elem, "data", "", "", "tile,chunk", "encoding,compression");
    const auto encoding = DataUtils::GetAttributeAsString(elem, "encoding");
    const auto compression = DataUtils::GetAttributeAsString(elem, "compression");
    const auto is_xml = encoding.empty();
    const auto is_csv = encoding == std::string{"csv"};
    const auto is_base64 = encoding == "base64";
    const auto is_base64gzip = is_base64 && compression == "gzip";
    const auto is_base64zlib = is_base64 && compression == "zlib";
    const auto is_base64zstd = is_base64 && compression == "zstd";
    if(is_xml) {
        g_theFileLogger->LogWarnLine("TMX Map data as XML is deprecated.");
        g_theFileLogger->Flush();
        std::size_t tile_index{ 0u };
        DataUtils::ForEachChildElement(elem, "tile", [layer, &tile_index](const XMLElement& tile_elem) {
            if(DataUtils::HasAttribute(tile_elem, "gid")) {
                const auto tile_gid = DataUtils::ParseXmlAttribute(tile_elem, "gid", 0);
                if(auto* tile = layer->GetTile(tile_index); tile != nullptr) {
                    if(tile_gid > 0 && tile_gid != std::size_t(-1)) {
                        tile->ChangeTypeFromId(tile_gid);
                    } else {
                        tile->ChangeTypeFromName("void");
                    }
                } else {
                    ERROR_AND_DIE("Too many tiles.");
                }
            } else {
                if(auto* tile = layer->GetTile(tile_index); tile != nullptr) {
                    tile->ChangeTypeFromName("void");
                } else {
                    ERROR_AND_DIE("Too many tiles.");
                }
            }
            ++tile_index;
        });
    } else if(is_csv) {
        const auto data_text = StringUtils::RemoveAllWhitespace(DataUtils::GetElementTextAsString(elem));
        std::size_t tile_index{ 0u };
        for(const auto& gid : StringUtils::Split(data_text)) {
            if(auto* tile = layer->GetTile(tile_index); tile != nullptr) {
                const auto gidAsId = static_cast<std::size_t>(std::stoull(gid));
                if(gidAsId > 0) {
                    tile->ChangeTypeFromId(gidAsId - firstgid);
                }
            } else {
                ERROR_AND_DIE("Too many tiles.");
            }
            ++tile_index;
        }
    } else if(is_base64) {
        const auto encoded_data_text = StringUtils::RemoveAllWhitespace(DataUtils::GetElementTextAsString(elem));
        auto output = std::vector<uint8_t>{};
        FileUtils::Base64::Decode(encoded_data_text, output);
        const auto width  = static_cast<std::size_t>(layer->tileDimensions.x);
        const auto height = static_cast<std::size_t>(layer->tileDimensions.y);
        const auto valid_data_size = width * height * std::size_t{ 4u };
        const auto err_msg = std::format("Invalid decoded Layer data: Size of data ({}) does not equal {} * {} * 4 or {}", output.size(), width, height, valid_data_size);
        GUARANTEE_OR_DIE(output.size() == valid_data_size, err_msg.c_str());

        std::size_t tile_index{ 0u };
        constexpr auto flag_flipped_horizontally = uint32_t{ 0x80000000u };
        constexpr auto flag_flipped_vertically   = uint32_t{ 0x40000000u };
        constexpr auto flag_flipped_diagonally   = uint32_t{ 0x20000000u };
        constexpr auto flag_rotated_hexagonal_120 = uint32_t{ 0x10000000u };
        constexpr auto flag_mask = flag_flipped_horizontally | flag_flipped_vertically | flag_flipped_diagonally | flag_rotated_hexagonal_120;
        for(std::size_t y = 0; y < height; ++y) {
            for(std::size_t x = 0; x < width; ++x) {
                auto gid = output[tile_index + 0] << 0  |
                           output[tile_index + 1] << 8  |
                           output[tile_index + 2] << 16 |
                           output[tile_index + 3] << 24;
                
                tile_index += 4;

                gid &= ~flag_mask;
                if(auto* tile = layer->GetTile(layer->GetTileIndex(x, y)); tile != nullptr) {
                    if(firstgid <= gid) {
                        tile->ChangeTypeFromId(static_cast<std::size_t>(gid) - firstgid);
                    }
                }
            }
        }
    } else if(is_base64gzip) {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    } else if(is_base64zlib) {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    } else if(is_base64zstd) {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    } else {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    }
}

void Map::LoadTmxTileset(const XMLElement& elem) noexcept {
    {
        const auto verify_version = [](const XMLElement& elem, std::string versionAttributeName, const std::string requiredVersionString) {
            if(const auto version_string = DataUtils::ParseXmlAttribute(elem, versionAttributeName, std::string{ "0.0" }); version_string != requiredVersionString) {
                const auto required_versions = StringUtils::Split(requiredVersionString, '.', false);
                const auto actual_versions = StringUtils::Split(version_string, '.', false);
                const auto required_major_version = std::stoi(required_versions[0]);
                const auto required_minor_version = std::stoi(required_versions[1]);
                const auto actual_major_version = std::stoi(actual_versions[0]);
                const auto actual_minor_version = std::stoi(actual_versions[1]);
                if(actual_major_version < required_major_version || (actual_major_version == required_major_version && actual_minor_version < required_minor_version)) {
                    const auto msg = std::format("ERROR: Attribute mismatch for \"{}\". Required: {} File: {}\n", versionAttributeName, requiredVersionString, version_string);
                    ERROR_AND_DIE(msg.c_str());
                }
            }
        };

        constexpr auto required_tsx_version = "1.9";
        verify_version(elem, "version", required_tsx_version);

        constexpr auto required_tiled_version = "1.9.2";
        verify_version(elem, "tiledversion", required_tiled_version);
    }

    TileDefinitionDesc desc{};
    const auto tilecount = DataUtils::ParseXmlAttribute(elem, "tilecount", 1);
    const auto columncount = DataUtils::ParseXmlAttribute(elem, "columns", 1);
    const auto width = columncount;
    const auto height = tilecount / columncount;
    if(const auto* xml_image = elem.FirstChildElement("image"); xml_image != nullptr) {
        //Attribute "id" is deprecated and unsupported. It is an error if it exists.
        DataUtils::ValidateXmlElement(*xml_image, "image", "", "source,width,height", "data", "id,format,trans");
        if(DataUtils::HasAttribute(elem, "id")) {
            g_theFileLogger->LogWarnLine(std::string{"Attribute \"id\" in the image element is deprecated and unsupported. Remove the attribute to suppress this message."});
        }
        auto src = std::filesystem::path{DataUtils::ParseXmlAttribute(*xml_image, "source", std::string{})};
        if(!src.has_parent_path() || (src.parent_path() != m_filepath.parent_path())) {
            src = std::filesystem::canonical(m_filepath.parent_path() / src);
        }
        src.make_preferred();
        GetGameAs<Game>()->_tileset_sheet = g_theRenderer->CreateSpriteSheet(src, width, height);
        //TileDefinition::CreateTileDefinition();

    }
    DataUtils::ForEachChildElement(elem, "tile", [&desc, width](const XMLElement& xml_tile) {
        const auto tile_idx = DataUtils::ParseXmlAttribute(xml_tile, "id", 0);
        desc.tileId = std::size_t( tile_idx );
        if(DataUtils::HasChild(xml_tile, "animation")) {
            desc.animated = true;
            if(const auto* xml_animation = xml_tile.FirstChildElement("animation"); xml_animation != nullptr) {
                TimeUtils::FPSeconds duration{0.0f};
                auto start_idx = 0;
                const auto length = static_cast<int>((std::max)(std::size_t{0u}, DataUtils::GetChildElementCount(*xml_animation, "frame")));
                desc.frame_length = length;
                if(DataUtils::HasChild(*xml_animation, "frame")) {
                    const auto xml_frame = xml_animation->FirstChildElement("frame");
                    start_idx = DataUtils::ParseXmlAttribute(*xml_frame, "tileid", 0);
                    GUARANTEE_OR_DIE(start_idx == tile_idx, "First animation tile index must match selected tile index.");
                    desc.anim_start_idx = start_idx;
                    DataUtils::ForEachChildElement(*xml_animation, "frame", [&duration](const XMLElement& frame_elem) {
                        duration += TimeUtils::FPMilliseconds{DataUtils::ParseXmlAttribute(frame_elem, "duration", 0)};
                    });
                    desc.anim_duration = duration.count();
                }
            }
        }
        if(DataUtils::HasChild(xml_tile, "properties")) {
            if(const auto* xml_properties = xml_tile.FirstChildElement("properties"); xml_properties != nullptr) {
                DataUtils::ForEachChildElement(*xml_properties, "property", [&](const XMLElement& property_elem) {
                    DataUtils::ValidateXmlElement(property_elem, "property", "", "name,value", "properties", "propertytype,type");
                    if(DataUtils::HasAttribute(property_elem, "type")) {
                        DataUtils::ValidateXmlAttribute(property_elem, "type", "bool,color,class,float,file,int,object,string");
                        if(const auto type_str = DataUtils::ParseXmlAttribute(property_elem, "type", std::string{ "string" }); !type_str.empty()) {
                            if(type_str == "bool") {
                                DataUtils::ValidateXmlAttribute(property_elem, "value", "true,false");
                                const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                                const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", false);
                                if(name == "allowDiagonalMovement") {
                                    desc.allow_diagonal_movement = value;
                                } else if(name == "opaque") {
                                    desc.opaque = value;
                                } else if(name == "solid") {
                                    desc.solid = value;
                                } else if(name == "visible") {
                                    desc.visible = value;
                                } else if(name == "transparent") {
                                    desc.transparent = value;
                                } else if(name == "invisible") {
                                    desc.visible = !value;
                                } else if(name == "entrance") {
                                    desc.is_entrance = value;
                                } else if(name == "exit") {
                                    desc.is_exit = value;
                                }
                            } else if(type_str == "int") {
                                const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                                const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", 0);
                                if(name == "light") {
                                    desc.light = value;
                                } else if(name == "selflight") {
                                    desc.self_illumination = value;
                                }
                            } else if(type_str == "color") {
                                //const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                                //const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", std::string{"#FFFFFFFF"});
                                //if(name == "tint") {
                                //    auto t = Rgba::NoAlpha;
                                //    t.SetRGBAFromARGB(value.empty() ? "#FFFFFFFF" : value);
                                //}
                            } else if(type_str == "file") {

                            } else if(type_str == "object") {

                            } else if(type_str == "class") {
                            }
                        }
                    } else { //No type attribute is a string
                        const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                        const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", std::string{});
                        if(name == "name") {
                            desc.name = value;
                        } else if(name == "animName") {
                            desc.animName = value;
                        } else if(name == "glyph") {
                            desc.glyph = value.empty() ? ' ' : value.front();
                        }
                    }
                });
            }
        }
        constexpr auto fmt = R"(<tileDefinition name="{:s}" index="[{},{}]"><glyph value="{}" /><animation name="{:s}"><animationset startindex="{}" framelength="{}" duration="{}" loop="true" /></animation></tileDefinition>)";
        const auto tileIdx = desc.tileId % width;
        const auto tileIdy = desc.tileId / width;
        const auto anim_str = std::vformat(fmt, std::make_format_args(desc.name, tileIdx, tileIdy, desc.glyph, desc.animName, desc.anim_start_idx, desc.frame_length, desc.anim_duration));
        tinyxml2::XMLDocument d;
        d.Parse(anim_str.c_str(), anim_str.size());
        if(const auto* xml_root = d.RootElement(); xml_root != nullptr) {
            if(auto* def = TileDefinition::CreateOrGetTileDefinition(*xml_root, GetGameAs<Game>()->_tileset_sheet); def && def->GetSprite() && !def->GetSprite()->GetMaterial()) {
                def->GetSprite()->SetMaterial(GetGameAs<Game>()->GetDefaultTileMaterial());
            }
        }
    });
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

void Map::LoadMaterialFromFile(const std::filesystem::path& src) noexcept {
    _default_tileMaterial = g_theRenderer->GetMaterial(src.string());
    _current_tileMaterial = _default_tileMaterial;
}

void Map::LoadGenerator(const XMLElement& elem) {
    const auto* xml_generator = elem.FirstChildElement("mapGenerator");
    DataUtils::ValidateXmlElement(*xml_generator, "mapGenerator", "", "type");
    _map_generator.SetParentMap(this);
    _map_generator.SetRootXmlElement(*xml_generator);
    _map_generator.Generate();
}

void Map::LoadTileDefinitionsForMap(const XMLElement& elem) {
    if(auto xml_tileset = elem.FirstChildElement("tiles")) {
        DataUtils::ValidateXmlElement(*xml_tileset, "tiles", "", "src");
        const auto src = DataUtils::ParseXmlAttribute(*xml_tileset, "src", std::string{});
        GUARANTEE_OR_DIE(!src.empty(), "Map tiles source is empty.");
        GetGameAs<Game>()->LoadTileDefinitionsFromFile(src);
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
            if(auto* feature = Feature::CreateFeature(this, elem); feature != nullptr) {
                //const auto instance = Feature::CreateInstanceFromFeature(feature);
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
