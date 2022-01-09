#include "Game/Editor/MapEditor.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/Game.hpp"

#include "Thirdparty/Imgui/imgui.h"

MapEditor::MapEditor(const std::filesystem::path& mapPath) noexcept
    : m_editorMap{mapPath}
{

}

void MapEditor::BeginFrame_Editor() noexcept {

}

void MapEditor::Update_Editor([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    ShowMainMenu(deltaSeconds);
    ShowViewport(deltaSeconds);
    ShowProperties(deltaSeconds);
}

void MapEditor::ShowMainMenu([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New...", "Ctrl+N")) {

            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {

            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {

            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {

            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void MapEditor::ShowViewport([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    const auto size = ImGui::GetContentRegionAvail();
    ImGui::Begin("Viewport");
    ImGui::Image(m_viewport_fb->GetTexture(), size, Vector2{}, Vector2{}, Rgba::White, Rgba::NavyBlue);
    ImGui::End();
}

void MapEditor::ShowProperties([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(ImGui::Begin("Properties", nullptr)) {
        ImGui::End();
    }
}

void MapEditor::Render_Editor() const noexcept {
    auto& renderer = ServiceLocator::get<IRendererService>();
    renderer.BeginRenderToBackbuffer();

    m_editorMap.Render();

#ifdef UI_DEBUG
    auto* game = GetGameAs<Game>();
    if(game->IsDebugging()) {
        m_editorMap.DebugRender();
    }
#endif

    auto& app = ServiceLocator::get<IAppService>();
    if(app.LostFocus()) {
        renderer.SetMaterial(g_theRenderer->GetMaterial("__2D"));
        renderer.DrawQuad2D(Vector2::Zero, Vector2::One, Rgba(0, 0, 0, 128));
    }

    renderer.BeginHUDRender(m_uiCamera, Vector2::Zero, static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight()));

    if(app.LostFocus()) {
        const auto w = static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowWidth());
        const auto h = static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight());
        renderer.DrawQuad2D(Matrix4::CreateScaleMatrix(Vector2{w, h}), Rgba{0.0f, 0.0f, 0.0f, 0.5f});
        renderer.DrawTextLine(Matrix4::I, renderer.GetFont("System32"), "PAUSED");
    }
}

void MapEditor::EndFrame_Editor() noexcept {
    /* DO NOTHING */
}

bool MapEditor::SerializeMap(const Map& /*map*/) const noexcept {
    return false;
}

bool MapEditor::DeserializeMap(Map& /*map*/) noexcept {
    return false;
}
