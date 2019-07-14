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
#include "Game/Actor.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

void Game::Initialize() {
    {
        auto dims = g_theRenderer->GetOutput()->GetDimensions();
        auto data = std::vector<Rgba>(dims.x * dims.y, Rgba::Magenta);
        auto fs = g_theRenderer->Create2DTextureFromMemory(data, dims.x, dims.y, BufferUsage::Gpu, BufferBindUsage::Render_Target | BufferBindUsage::Shader_Resource);
        g_theRenderer->RegisterTexture("__fullscreen", fs);
    }
    g_theRenderer->RegisterMaterialsFromFolder(std::string{ "Data/Materials" });
    _fullscreen_cb = std::unique_ptr<ConstantBuffer>(g_theRenderer->CreateConstantBuffer(&_fullscreen_data, sizeof(_fullscreen_data)));
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
    g_theRenderer->UpdateGameTime(deltaSeconds);
    Camera2D& base_camera = _map->camera;
    HandleDebugInput(base_camera);
    HandlePlayerInput(base_camera);

    {
        static bool value = true;
        if(g_theInputSystem->WasKeyJustPressed(KeyCode::Enter)) {
            value = !value;
        }
        if(DoFade(value)) {
            StopFade();
        }
    }

    base_camera.Update(deltaSeconds);
    _map->Update(deltaSeconds);
}

bool Game::DoFade(bool fadeIn) {
    static TimeUtils::FPSeconds curFadeTime{};
    if(fadeIn && _fullscreen_data.effectIndex != 0) {
        _fullscreen_data.effectIndex = 0;
        curFadeTime = curFadeTime.zero();
    } else if(!fadeIn && _fullscreen_data.effectIndex != 1) {
        _fullscreen_data.effectIndex = 1;
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.fadePercent = curFadeTime.count() / 5.0f;
    _fullscreen_data.fadePercent = std::clamp(_fullscreen_data.fadePercent, 0.0f, 1.0f);
    auto color = Rgba::Black.GetRgbaAsFloats();
    _fullscreen_data.effectIndex = !!fadeIn;
    _fullscreen_data.fadeColor[0] = color.x;
    _fullscreen_data.fadeColor[1] = color.y;
    _fullscreen_data.fadeColor[2] = color.z;
    _fullscreen_data.fadeColor[3] = color.w;
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);

    curFadeTime += g_theRenderer->GetGameFrameTime();
    return _fullscreen_data.fadePercent == 1.0f;
}

void Game::StopFade() {
    static TimeUtils::FPSeconds curFadeTime{};
    _fullscreen_data.effectIndex = -1;
    _fullscreen_data.fadePercent = 0.0f;
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::Render() const {
    g_theRenderer->SetTexture(nullptr); //Force bound texture to invalid.
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTarget(g_theRenderer->GetTexture("__fullscreen"));
    g_theRenderer->ClearColor(Rgba::Olive);
    g_theRenderer->ClearDepthStencilBuffer();


    g_theRenderer->SetViewportAsPercent();

    _map->Render(*g_theRenderer);

    if(_show_grid || _show_world_bounds) {
        _map->DebugRender(*g_theRenderer);
    }

    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearColor(Rgba::NoAlpha);
    g_theRenderer->ClearDepthStencilBuffer();
    g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("Fullscreen"));
    g_theRenderer->SetConstantBuffer(3, _fullscreen_cb.get());
    std::vector<Vertex3D> vbo =
    {
        Vertex3D{Vector3::ZERO}
        ,Vertex3D{Vector3{3.0f, 0.0f, 0.0f}}
        ,Vertex3D{Vector3{0.0f, 3.0f, 0.0f}}
    };
    g_theRenderer->Draw(PrimitiveType::Triangles, vbo, 3);

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

void Game::HandlePlayerInput(Camera2D& base_camera) {
    _player_requested_wait = false;
    const bool is_right = g_theInputSystem->WasKeyJustPressed(KeyCode::D) ||
                          g_theInputSystem->WasKeyJustPressed(KeyCode::Right) ||
                          g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad6);
    const bool is_left = g_theInputSystem->WasKeyJustPressed(KeyCode::A) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Left) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad4);

    const bool is_up = g_theInputSystem->WasKeyJustPressed(KeyCode::W) ||
                       g_theInputSystem->WasKeyJustPressed(KeyCode::Up) ||
                       g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad8);
    const bool is_down = g_theInputSystem->WasKeyJustPressed(KeyCode::S) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Down) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad2);

    const bool is_upright = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad9) || (is_right && is_up);
    const bool is_upleft = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad7) || (is_left && is_up);
    const bool is_downright = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad3) || (is_right && is_down);
    const bool is_downleft = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad1) || (is_left && is_down);

    const bool is_shift = g_theInputSystem->IsKeyDown(KeyCode::Shift);
    const bool is_rest = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad5)
                         || g_theInputSystem->IsKeyDown(KeyCode::NumPad5)
                         || g_theInputSystem->WasKeyJustPressed(KeyCode::Z)
                         || g_theInputSystem->IsKeyDown(KeyCode::Z);

    if(is_shift) {
        if(is_right) {
            base_camera.position += Vector2::X_AXIS;
        } else if(is_left) {
            base_camera.position += -Vector2::X_AXIS;
        }

        if(is_up) {
            base_camera.position += -Vector2::Y_AXIS;
        } else if(is_down) {
            base_camera.position += Vector2::Y_AXIS;
        }
        return;
    }

    if(is_rest) {
        _player_requested_wait = true;
        return;
    }
    auto player = dynamic_cast<Actor*>(_map->player);
    if(is_upright) {
        player->MoveNorthEast();
    } else if(is_upleft) {
        player->MoveNorthWest();
    } else if(is_downright) {
        player->MoveSouthEast();
    } else if(is_downleft) {
        player->MoveSouthWest();
    } else {
        if(is_right) {
            player->MoveEast();
        } else if(is_left) {
            player->MoveWest();
        }

        if(is_up) {
            player->MoveNorth();
        } else if(is_down) {
            player->MoveSouth();
        }
    }
    if(!_map->IsEntityInView(dynamic_cast<Entity*>(player))) {
        _map->FocusEntity(player);
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
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::O)) {
        _map->SetPriorityLayer(static_cast<std::size_t>(MathUtils::GetRandomIntLessThan(static_cast<int>(_map->GetLayerCount()))));
    }

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::B)) {
        base_camera.trauma += 1.0f;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::R)) {
        const auto layer_count = _map->GetLayerCount();
        for(std::size_t i = 0; i < layer_count; ++i) {
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
    ImGui::Text("Tile Inspector");
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
    ss << "\nAnimated: " << (cur_entity->def->is_animated ? "true" : "false");
    ImGui::Text(ss.str().c_str());
    const auto tex_coords = cur_sprite->GetCurrentTexCoords();
    const auto dims = Vector2::ONE * 100.0f;
    ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
}

void Game::ShowEntityInspectorInventoryColumnUI(const Entity* cur_entity) {
    if(!cur_entity) {
        return;
    }
    std::ostringstream ss;
    if(cur_entity->inventory.empty()) {
        ss << "Inventory: None";
        ImGui::Text(ss.str().c_str());
        return;
    }
    ss << "Inventory:";
    for(const auto* item : cur_entity->inventory) {
        ss << '\n' << item->GetName();
    }
    ImGui::Text(ss.str().c_str());
}
