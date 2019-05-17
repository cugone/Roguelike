#include "Game/Game.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Map.hpp"
#include "Game/Layer.hpp"

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
    _map->camera.position = _map->GetMaxDimensions() * 0.5f;
}

void Game::BeginFrame() {
    _map->BeginFrame();
}

void Game::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(_show_debug_window) {
        ShowDebugUI();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        g_theApp->SetIsQuitting(true);
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        _show_debug_window = !_show_debug_window;
    }
    Camera2D& base_camera = _map->camera;
    if(g_theInputSystem->IsKeyDown(KeyCode::D)) {
        base_camera.Translate(Vector2{1.0f, 0.0f} * _cam_speed);
    } else if(g_theInputSystem->IsKeyDown(KeyCode::A)) {
        base_camera.Translate(Vector2{-1.0f, 0.0f} *_cam_speed);
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::W)) {
        base_camera.Translate(Vector2{0.0f, -1.0f} *_cam_speed);
    } else if(g_theInputSystem->IsKeyDown(KeyCode::S)) {
        base_camera.Translate(Vector2{0.0f, 1.0f} *_cam_speed);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Right)) {
        base_camera.Translate(Vector2{1.0f, 0.0f} * _cam_speed);
    } else if(g_theInputSystem->WasKeyJustPressed(KeyCode::Left)) {
        base_camera.Translate(Vector2{-1.0f, 0.0f} *_cam_speed);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Up)) {
        base_camera.Translate(Vector2{0.0f, -1.0f} *_cam_speed);
    } else if(g_theInputSystem->WasKeyJustPressed(KeyCode::Down)) {
        base_camera.Translate(Vector2{0.0f, 1.0f} *_cam_speed);
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

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::R)) {
        const auto count = _map->GetLayerCount();
        for(std::size_t i = 0; i < count; ++i) {
            auto* cur_layer = _map->GetLayer(i);
            if(cur_layer) {
                cur_layer->viewHeight = cur_layer->GetDefaultViewHeight();
            }
        }
    }

    _map->camera.Update(deltaSeconds);

    _map->Update(deltaSeconds);

}

void Game::Render() const {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Olive);

    g_theRenderer->SetViewportAsPercent();

    _map->Render(*g_theRenderer);

    if(_debug || _show_grid) {
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

    {
        auto* f = g_theRenderer->GetFont("System32");
        std::ostringstream ss;
        ss << "Cam Pos: " << _map->camera.position;
        auto S = Matrix4::I;
        auto R = Matrix4::I;
        auto T = Matrix4::CreateTranslationMatrix(Vector2(0.0f, f->GetLineHeight() * 1.0f));
        auto M = T * R * S;
        g_theRenderer->SetModelMatrix(M);
        g_theRenderer->DrawMultilineText(f, ss.str(), Rgba::White);
    }

}

void Game::EndFrame() {

}

void Game::ShowDebugUI() {
#ifdef UI_DEBUG
    ImGui::Begin("Tile Debugger", &_show_debug_window, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Checkbox("Grid", &_show_grid);
        ImGui::SliderFloat("Camera Shake Angle", &_max_shake_angle, 0.0f, 90.0f);
        ImGui::SliderFloat("Camera Shake X Offset", &_max_shake_x, 0.0f, 0.00000025f, "%.8f");
        ImGui::SliderFloat("Camera Shake Y Offset", &_max_shake_y, 0.0f, 0.00000025f, "%.8f");
        GAME_OPTION_MAX_SHAKE_ANGLE = _max_shake_angle;
        GAME_OPTION_MAX_SHAKE_OFFSET_H = _max_shake_x;
        GAME_OPTION_MAX_SHAKE_OFFSET_V = _max_shake_y;
    }
    ImGui::End();
#endif
}
