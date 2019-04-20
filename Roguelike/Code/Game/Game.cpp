#include "Game/Game.hpp"

#include "Engine/Math/Vector2.hpp"

#include "Engine/Core/KerningFont.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

void Game::Initialize() {
    _world_camera.position = Vector2{ 8.0f, 4.0f };
}

void Game::BeginFrame() {

}

void Game::Update(TimeUtils::FPSeconds /*deltaSeconds*/) {
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
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::D)) {
        _world_camera.Translate(Vector2{1.0f, 0.0f} * _cam_speed);
    } else if(g_theInputSystem->WasKeyJustPressed(KeyCode::A)) {
        _world_camera.Translate(Vector2{-1.0f, 0.0f} *_cam_speed);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::W)) {
        _world_camera.Translate(Vector2{0.0f, -1.0f} *_cam_speed);
    } else if(g_theInputSystem->WasKeyJustPressed(KeyCode::S)) {
        _world_camera.Translate(Vector2{0.0f, 1.0f} *_cam_speed);
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::Up)) {
        GAME_OPTION_TILE_VIEW_HEIGHT += 1.0f;
    } else if(g_theInputSystem->IsKeyDown(KeyCode::Down)) {
        GAME_OPTION_TILE_VIEW_HEIGHT -= 1.0f;
    } else if(g_theInputSystem->IsKeyDown(KeyCode::Right)) {
        GAME_OPTION_TILE_VIEW_HEIGHT = 10.0f;
    }

}

void Game::Render() const {
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->ClearColor(Rgba::Olive);

    g_theRenderer->SetViewport(0, 0, static_cast<unsigned int>(GRAPHICS_OPTION_WINDOW_WIDTH), static_cast<unsigned int>(GRAPHICS_OPTION_WINDOW_HEIGHT));

    //3D View / CAMERA
    const float world_view_height = GAME_OPTION_TILE_VIEW_HEIGHT;
    const float world_view_width = world_view_height * _world_camera.GetAspectRatio();
    const float world_view_half_height = world_view_height * 0.5f;
    const float world_view_half_width = world_view_width * 0.5f;
    auto world_leftBottom = Vector2{ -world_view_half_width, world_view_half_height };
    auto world_rightTop = Vector2{ world_view_half_width, -world_view_half_height };
    auto world_nearFar = Vector2{ 0.0f, 1.0f };
    _world_camera.SetupView(world_leftBottom, world_rightTop, world_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_world_camera);

    g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
    g_theRenderer->SetModelMatrix(Matrix4::CreateScaleMatrix(Vector2::ONE * 2.0f));
    g_theRenderer->DrawQuad();

    //2D View / HUD
    const float ui_view_height = GRAPHICS_OPTION_WINDOW_HEIGHT;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const float ui_view_half_height = ui_view_height * 0.5f;
    const float ui_view_half_width = ui_view_width * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_width, ui_view_half_height };
    auto ui_rightTop = Vector2{ ui_view_half_width, -ui_view_half_height };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = Vector2{ ui_view_half_width, ui_view_half_height };
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

    {
        auto* f = g_theRenderer->GetFont("System32");
        std::ostringstream ss;
        ss << "Cam Pos: " << _world_camera.position << "\nTile View Height: " << GAME_OPTION_TILE_VIEW_HEIGHT;
        auto S = Matrix4::I;
        auto R = Matrix4::I;
        auto T = Matrix4::CreateTranslationMatrix(Vector2(0.0f, f->GetLineHeight() * 1.0f));
        auto M = T * R * S;
        g_theRenderer->SetModelMatrix(M);
        g_theRenderer->DrawMultilineText(f, ss.str(), Rgba::Black);
    }

}

void Game::EndFrame() {

}

void Game::ShowDebugUI() {
    ImGui::Begin("Tile Debugger", &_show_debug_window, ImGuiWindowFlags_AlwaysAutoResize);
    {
        ImGui::Checkbox("Grid", &_show_grid);
    }
    ImGui::End();
}
