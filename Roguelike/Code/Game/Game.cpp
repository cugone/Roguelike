#include "Game/Game.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Texture.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

void Game::Initialize() {
    g_theRenderer->RegisterMaterialsFromFolder(std::string{ "Data/Materials" });
    {
        auto str_path = std::string{ "Data/Definitions/Map00.xml" };
        if(FileUtils::IsSafeReadPath(str_path)) {
            std::string str_buffer{};
            if(FileUtils::ReadBufferFromFile(str_buffer, str_path)) {
                tinyxml2::XMLDocument xml_doc;
                xml_doc.Parse(str_buffer.c_str(), str_buffer.size());
                _map = std::make_unique<Map>(*xml_doc.RootElement());
            }
        }
    }
    _map->camera.position = _map->CalcMaxDimensions() * 0.5f;
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
    HandlePlayerInput(base_camera);
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

void Game::HandlePlayerInput(Camera2D& base_camera) {
    const bool is_right = g_theInputSystem->IsKeyDown(KeyCode::D) ||
                          g_theInputSystem->WasKeyJustPressed(KeyCode::Right);
    const bool is_left = g_theInputSystem->IsKeyDown(KeyCode::A) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Left);
    if(is_right) {
        base_camera.Translate(Vector2{ 1.0f, 0.0f } * _cam_speed);
    } else if(is_left) {
        base_camera.Translate(Vector2{ -1.0f, 0.0f } * _cam_speed);
    }

    const bool is_up = g_theInputSystem->IsKeyDown(KeyCode::W) ||
                       g_theInputSystem->WasKeyJustPressed(KeyCode::Up);
    const bool is_down = g_theInputSystem->IsKeyDown(KeyCode::S) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Down);

    if(is_up) {
        base_camera.Translate(Vector2{ 0.0f, -1.0f } * _cam_speed);
    } else if(is_down) {
        base_camera.Translate(Vector2{ 0.0f, 1.0f } * _cam_speed);
    }
}

void Game::HandleDebugInput(Camera2D &base_camera) {
    if(_show_debug_window) {
        ShowDebugUI();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        _show_debug_window = !_show_debug_window;
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

void Game::ShowDebugUI() {
    if(ImGui::Begin("Tile Debugger", &_show_debug_window, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ShowBoundsColoringUI();
        ShowTileInspectorUI();
    }
    ImGui::End();
}

void Game::ShowBoundsColoringUI() {
    ImGui::Checkbox("World Grid", &_show_grid);
    if(ImGui::ColorEdit4("Grid Color##Picker", _grid_color, ImGuiColorEditFlags_None)) {
        _map->SetDebugGridColor(_grid_color);
    }
    ImGui::Checkbox("World Bounds", &_show_world_bounds);
}

void Game::ShowTileInspectorUI() {
    const auto mouse_pos = g_theInputSystem->GetCursorWindowPosition(*g_theRenderer->GetOutput()->GetWindow());
    const auto& picked_tiles = _map->PickTilesFromMouseCoords(mouse_pos);
    if(picked_tiles.empty()) {
        ImGui::Text("Tile Inspector: None");
        return;
    }
    ImGui::Text("Tile Inspector");
    const auto max_layers = std::size_t{9u};
    const auto tiles_per_row = std::size_t{3u};
    const auto picked_count = picked_tiles.size();
    for(std::size_t i = 0; i < max_layers; ++i) {
        const auto* cur_tile = i < picked_count ? picked_tiles[i] : nullptr;
        if(const auto* cur_def = cur_tile ? cur_tile->GetDefinition() : TileDefinition::GetTileDefinitionByName("void")) {
            if(const auto* cur_sheet = cur_def->GetSheet()) {
                const auto tex_coords = cur_sheet->GetTexCoordsFromSpriteCoords(cur_def->index);
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sheet->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(!i || (i % tiles_per_row) < tiles_per_row - 1) {
                    ImGui::SameLine();
                }
            }
        }
    }
}
