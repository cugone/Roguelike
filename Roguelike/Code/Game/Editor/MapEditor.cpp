#include "Game/Editor/MapEditor.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IAppService.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/Game.hpp"

#include "Thirdparty/Imgui/imgui.h"

MapEditor::MapEditor(IntVector2 dimensions /*= IntVector2{ min_map_width, min_map_height }*/) noexcept
: m_editorMap{dimensions}
{
    m_editorMap.DebugDisableLighting(true);
    m_editorMap.DebugShowInvisibleTiles(true);
}

MapEditor::MapEditor(const std::filesystem::path& mapPath) noexcept
    : m_editorMap{mapPath}
{
    m_editorMap.DebugDisableLighting(true);

}

void MapEditor::BeginFrame_Editor() noexcept {
    ImGui::DockSpaceOverViewport();
}

void MapEditor::Update_Editor([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    m_editorMap.Update(deltaSeconds);
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
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S")) {

            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {

            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "")) {
                GetGameAs<Game>()->ChangeGameState(GameState::Editor);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void MapEditor::ShowViewport([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    ImGui::Begin("Viewport");
    const auto viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x != m_ViewportWidth || viewportSize.y != m_ViewportHeight) {
        m_ViewportWidth = static_cast<uint32_t>(std::floor(viewportSize.x));
        m_ViewportHeight = static_cast<uint32_t>(std::floor(viewportSize.y));
        m_viewport_fb->Resize(m_ViewportWidth, m_ViewportHeight);
    }
    ImGui::Image(m_viewport_fb->GetTexture(), viewportSize, Vector2::Zero, Vector2::One, Rgba::White, Rgba::NavyBlue);
    ImGui::End();
}

void MapEditor::ShowProperties([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if(ImGui::Begin("Properties", nullptr)) {
        //ImGui::Text("Tileset:");
        //ImGui::SameLine();
        static std::string tileset_str{ default_tile_definition_src.string()};
        ImGui::InputText("Tileset##MapEditorTileset", &tileset_str);
        ImGui::End();
    }
}

void MapEditor::Render_Editor() const noexcept {
    auto* renderer = ServiceLocator::get<IRendererService>();

    renderer->BeginRender(m_viewport_fb->GetTexture(), Rgba::Black, m_viewport_fb->GetDepthStencil());

    renderer->SetOrthoProjectionFromCamera(Camera3D{ m_editorCamera.GetCamera() });
    renderer->SetCamera(m_editorCamera.GetCamera());

    m_editorMap.Render();

#ifdef UI_DEBUG
    auto* game = GetGameAs<Game>();
    if(game->IsDebugging()) {
        m_editorMap.DebugRender();
    }
#endif

    renderer->BeginRenderToBackbuffer();

    auto* app = ServiceLocator::get<IAppService>();
    if(app->LostFocus()) {
        renderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        renderer->DrawQuad2D(Vector2::Zero, Vector2::One, Rgba(0, 0, 0, 128));
    }

    renderer->BeginHUDRender(m_uiCamera, Vector2::Zero, static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight()));

    if(app->LostFocus()) {
        const auto w = static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowWidth());
        const auto h = static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight());
        renderer->DrawQuad2D(Matrix4::CreateScaleMatrix(Vector2{w, h}), Rgba{0.0f, 0.0f, 0.0f, 0.5f});
        renderer->DrawTextLine(Matrix4::I, renderer->GetFont("System32"), "PAUSED");
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
