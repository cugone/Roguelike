#include "Game/Game.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Texture.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Entity.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

void Game::Initialize() {
    g_theRenderer->RegisterMaterialsFromFolder(std::string{ "Data/Materials" });
    LoadMaps();
    _map->camera.position = _map->CalcMaxDimensions() * 0.5f;
}

void Game::LoadMaps() {
    auto str_path = std::string{ "Data/Definitions/Map00.xml" };
    if(FileUtils::IsSafeReadPath(str_path)) {
        std::string str_buffer{};
        if(FileUtils::ReadBufferFromFile(str_buffer, str_path)) {
            tinyxml2::XMLDocument xml_doc;
            xml_doc.Parse(str_buffer.c_str(), str_buffer.size());
            _map = std::make_unique<Map>(*g_theRenderer, *xml_doc.RootElement());
        }
    }
}

void Game::BeginFrame() {
    _map->BeginFrame();
}

void Game::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        g_theApp->SetIsQuitting(true);
        return;
    }
    Camera2D& base_camera = _map->camera;
    HandleDebugInput(base_camera);
    HandlePlayerInput();
    base_camera.Update(deltaSeconds);
    _map->Update(deltaSeconds);
}

void Game::Render() const {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Olive);

    g_theRenderer->SetViewportAsPercent();

    _map->Render(*g_theRenderer);

    if(_show_grid || _show_world_bounds) {
        _map->DebugRender(*g_theRenderer);
    }

    //2D View / HUD
    const float ui_view_height = GRAPHICS_OPTION_WINDOW_HEIGHT;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ui_view_width, ui_view_height};
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_extents.x, ui_view_half_extents.y };
    auto ui_rightTop = Vector2{ ui_view_half_extents.x, -ui_view_half_extents.y };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

}

void Game::EndFrame() {

}

void Game::HandlePlayerInput() {
    const bool is_right = g_theInputSystem->WasKeyJustPressed(KeyCode::D) ||
                          g_theInputSystem->WasKeyJustPressed(KeyCode::Right);
    const bool is_left = g_theInputSystem->WasKeyJustPressed(KeyCode::A) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Left);
    const bool is_shift = g_theInputSystem->IsKeyDown(KeyCode::Shift);
    if(is_right) {
        _map->player->MoveEast();
    } else if(is_left) {
        _map->player->MoveWest();
    }

    const bool is_up = g_theInputSystem->WasKeyJustPressed(KeyCode::W) ||
                       g_theInputSystem->WasKeyJustPressed(KeyCode::Up);
    const bool is_down = g_theInputSystem->WasKeyJustPressed(KeyCode::S) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Down);

    if(is_up) {
        _map->player->MoveNorth();
    } else if(is_down) {
        _map->player->MoveSouth();
    }
    if(!_map->IsEntityInView(_map->player)) {
        _map->FocusEntity(_map->player);
    }
}

void Game::HandleDebugInput(Camera2D& base_camera) {
    if(_show_debug_window) {
        ShowDebugUI();
    }
    HandleDebugKeyboardInput(base_camera);
    HandleDebugMouseInput(base_camera);
}

void Game::HandleDebugKeyboardInput(Camera2D& base_camera) {
    if(g_theUISystem->GetIO().WantCaptureKeyboard) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        _show_debug_window = !_show_debug_window;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F2)) {
        _show_grid = !_show_grid;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F3)) {
        _show_world_bounds = !_show_world_bounds;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F4)) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::LeftBracket)) {
        const auto count = _map->GetLayerCount();
        for(std::size_t i = 0; i < count; ++i) {
            auto* cur_layer = _map->GetLayer(i);
            if(cur_layer) {
                ++cur_layer->viewHeight;
            }
        }
    } else if(g_theInputSystem->WasKeyJustPressed(KeyCode::RightBracket)) {
        const auto count = _map->GetLayerCount();
        for(std::size_t i = 0; i < count; ++i) {
            auto* cur_layer = _map->GetLayer(i);
            if(cur_layer) {
                --cur_layer->viewHeight;
            }
        }
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::B)) {
        base_camera.trauma += 1.0f;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::R)) {
        const auto count = _map->GetLayerCount();
        for(std::size_t i = 0; i < count; ++i) {
            auto* cur_layer = _map->GetLayer(i);
            if(cur_layer) {
                cur_layer->viewHeight = cur_layer->GetDefaultViewHeight();
            }
        }
    }
}

void Game::HandleDebugMouseInput(Camera2D& /*base_camera*/) {
    if(g_theUISystem->GetIO().WantCaptureMouse) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::LButton)) {
        const auto& picked_tiles = DebugGetTilesFromMouse();
        _debug_has_picked_tile_with_click = _show_tile_debugger && !picked_tiles.empty();
        _debug_has_picked_entity_with_click = _show_entity_debugger && !picked_tiles.empty();
        if(_debug_has_picked_tile_with_click) {
            _debug_inspected_tiles = picked_tiles;
        }
        if(_debug_has_picked_entity_with_click) {
            _debug_inspected_entity = picked_tiles[0]->entity;
            _debug_has_picked_entity_with_click = _debug_inspected_entity;
        }
    }
}

void Game::ShowDebugUI() {
    ImGui::SetNextWindowSize(Vector2{ 350.0f, 500.0f }, ImGuiCond_Always);
    if(ImGui::Begin("Debugger", &_show_debug_window, ImGuiWindowFlags_AlwaysAutoResize)) {
        ShowBoundsColoringUI();
        ShowTileDebuggerUI();
        ShowEntityDebuggerUI();
    }
    ImGui::End();
}

void Game::ShowTileDebuggerUI() {
    _show_tile_debugger = ImGui::CollapsingHeader("Tile");
    if(_show_tile_debugger) {
        ShowTileInspectorUI();
    }
}

void Game::ShowEntityDebuggerUI() {
    _show_entity_debugger = ImGui::CollapsingHeader("Entity");
    if(_show_entity_debugger) {
        ShowEntityInspectorUI();
    }
}

void Game::ShowBoundsColoringUI() {
    if(ImGui::CollapsingHeader("World")) {
        ImGui::Checkbox("World Grid", &_show_grid);
        ImGui::SameLine();
        if(ImGui::ColorEdit4("Grid Color##Picker", _grid_color, ImGuiColorEditFlags_NoLabel)) {
            _map->SetDebugGridColor(_grid_color);
        }
        ImGui::Checkbox("World Bounds", &_show_world_bounds);
        ImGui::Checkbox("Show All Entities", &_show_all_entities);
    }
}

void Game::ShowTileInspectorUI() {
    const auto& picked_tiles = DebugGetTilesFromMouse();
    bool has_tile = !picked_tiles.empty();
    bool has_selected_tile = _debug_has_picked_tile_with_click && !_debug_inspected_tiles.empty();
    bool shouldnt_show_inspector = !has_tile && !has_selected_tile;
    if(shouldnt_show_inspector) {
        ImGui::Text("Tile Inspector: None");
        return;
    }
    ImGui::Text("Entity Inspector");
    ImGui::SameLine();
    if(ImGui::Button("Unlock Tile")) {
        _debug_has_picked_tile_with_click = false;
        _debug_inspected_tiles.clear();
    }
    const auto max_layers = std::size_t{ 9u };
    const auto tiles_per_row = std::size_t{ 3u };
    auto picked_count = picked_tiles.size();
    if(_debug_has_picked_tile_with_click) {
        picked_count = _debug_inspected_tiles.size();
        std::size_t i = 0u;
        for(const auto cur_tile : _debug_inspected_tiles) {
            const auto* cur_def = cur_tile ? cur_tile->GetDefinition() : TileDefinition::GetTileDefinitionByName("void");
            if(const auto* cur_sprite = cur_def->GetSprite()) {
                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(!i || (i % tiles_per_row) < tiles_per_row - 1) {
                    ImGui::SameLine();
                }
            }
            ++i;
        }
        for(i; i < max_layers; ++i) {
            const auto* cur_def = TileDefinition::GetTileDefinitionByName("void");
            if(const auto* cur_sprite = cur_def->GetSprite()) {
                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(!i || (i % tiles_per_row) < tiles_per_row - 1) {
                    ImGui::SameLine();
                }
            }
        }
    } else {
        for(std::size_t i = 0; i < max_layers; ++i) {
            const Tile *cur_tile = i < picked_count ? picked_tiles[i] : nullptr;
            const auto* cur_def = cur_tile ? cur_tile->GetDefinition() : TileDefinition::GetTileDefinitionByName("void");
            if(const auto* cur_sprite = cur_def->GetSprite()) {
                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(!i || (i % tiles_per_row) < tiles_per_row - 1) {
                    ImGui::SameLine();
                }
            }
        }
    }
    ImGui::NewLine();
}

std::vector<Tile*> Game::DebugGetTilesFromMouse() {
    if(g_theUISystem->GetIO().WantCaptureMouse) {
        return {};
    }
    const auto mouse_pos = g_theInputSystem->GetCursorWindowPosition(*g_theRenderer->GetOutput()->GetWindow());
    if(_debug_has_picked_tile_with_click) {
        static std::vector<Tile*> picked_tiles{};
        if(!_debug_has_picked_tile_with_click) {
            picked_tiles.clear();
        }
        if(_debug_has_picked_tile_with_click) {
            picked_tiles = _map->PickTilesFromMouseCoords(mouse_pos);
        }
        bool tile_has_entity = !picked_tiles.empty() && picked_tiles[0]->entity;
        if(tile_has_entity && _debug_has_picked_entity_with_click) {
            _debug_inspected_entity = picked_tiles[0]->entity;
        }
        return picked_tiles;
    }
    return _map->PickTilesFromMouseCoords(mouse_pos);
}

void Game::ShowEntityInspectorUI() {
    const auto& picked_tiles = DebugGetTilesFromMouse();
    bool has_entity = (!picked_tiles.empty() && picked_tiles[0]->entity);
    bool has_selected_entity = _debug_has_picked_entity_with_click && _debug_inspected_entity;
    bool shouldnt_show_inspector = !has_entity && !has_selected_entity;
    if(shouldnt_show_inspector) {
        ImGui::Text("Entity Inspector: None");
        return;
    }
    if(const auto* cur_entity = _debug_inspected_entity ? _debug_inspected_entity : picked_tiles[0]->entity) {
        if(const auto* cur_sprite = cur_entity->sprite) {
            ImGui::Text("Entity Inspector");
            ImGui::SameLine();
            if(ImGui::Button("Unlock Entity")) {
                _debug_has_picked_entity_with_click = false;
                _debug_inspected_entity = nullptr;
            }
            ImGui::Columns(2, "EntityInspectorColumns");
            ShowEntityInspectorEntityColumnUI(cur_entity, cur_sprite);
            ImGui::NextColumn();
            ShowEntityInspectorInventoryColumnUI(cur_entity);
        }
    }
}

void Game::ShowEntityInspectorEntityColumnUI(const Entity* cur_entity, const AnimatedSprite* cur_sprite) {
    std::ostringstream ss;
    ss << "Name: " << cur_entity->name;
    ss << "\nInvisible: " << (cur_entity->def->is_invisible ? "true" : "false");
    ImGui::Text(ss.str().c_str());
    const auto tex_coords = cur_sprite->GetCurrentTexCoords();
    const auto dims = Vector2::ONE * 100.0f;
    ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
}

void Game::ShowEntityInspectorInventoryColumnUI(const Entity* /*cur_entity*/) {
    std::ostringstream ss;
    ss << "Inventory:";
    ss << "\nDummyText0";
    ss << "\nDummyText1";
    ss << "\nDummyText2";
    ss << "\nDummyText3";
    ImGui::Text(ss.str().c_str());
}
