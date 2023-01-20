#include "Game/Editor/MapEditor.hpp"

#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Platform/PlatformUtils.hpp"

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
    , m_map_path{mapPath}
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

void MapEditor::DoSave() noexcept {
    if(SerializeMap(m_editorMap, m_map_path)) {
        m_hasUnsavedChanges = false;
    }
}

void MapEditor::DoSaveAs() noexcept {
    if(const auto sfdResult = FileDialogs::SaveFile("Map file (*.xml)\0*.xml\0Tiled Map (*.tmx)\0*.tmx\0\0"); !sfdResult.empty()) {
        m_map_path = sfdResult;
        if(SerializeMap(m_editorMap, m_map_path)) {
            m_hasUnsavedChanges = false;
        }
    }
}

void MapEditor::ShowMainMenu([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New...", "Ctrl+N")) {

            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                //if(const auto ofdResult = FileDialogs::OpenFile("Map file (*.xml)\0*.xml\0\0"); !ofdResult.empty()) {
                //    mapPath = ofdResult;
                //    m_requested_map_to_load = std::filesystem::path{ mapPath };
                //    LoadUI();
                //    LoadItems();
                //    LoadEntities();
                //    ChangeGameState(GameState::Editor_Main);
                //}
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save", "Ctrl+S", nullptr, m_hasUnsavedChanges)) {
                m_map_path.empty() ? DoSaveAs() :  DoSave();
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S", nullptr, m_hasUnsavedChanges)) {
                DoSaveAs();
            }
            ImGui::Separator();
            if(ImGui::MenuItem("Import...", nullptr, nullptr)) {
                if(const auto ofdResult = FileDialogs::OpenFile(""); !ofdResult.empty()) {
                    if(DeserializeMap(m_editorMap, ofdResult)) {

                    }
                }
            }
            if(ImGui::MenuItem("Export...", nullptr, nullptr)) {
                if(const auto sfdResult = FileDialogs::SaveFile("Map file (*.xml)\0*.xml\0Tiled Map (*.tmx)\0*.tmx\0\0"); !sfdResult.empty()) {
                    if(SerializeMap(m_editorMap, sfdResult)) {

                    }
                }
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
    const auto viewport_name = m_hasUnsavedChanges ? std::vformat("* {}{}", std::make_format_args(m_map_path.stem(), m_map_path.extension())) : std::vformat("{}{}", std::make_format_args(m_map_path.stem(), m_map_path.extension()));
    if(!ImGui::Begin(viewport_name.c_str())) {
        ImGui::End();
        return;
    }
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
    if(!ImGui::Begin("Properties", nullptr)) {
        ImGui::End();
        return;
    }
    //ImGui::Text("Tileset:");
    //ImGui::SameLine();
    static std::string tileset_str{ default_tile_definition_src.string()};
    ImGui::InputText("Tileset##MapEditorTileset", &tileset_str);
    ImGui::End();
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

bool MapEditor::SerializeMap(const Map& map, std::filesystem::path filepath) const noexcept {
    {
        std::error_code ec{};
        if(filepath = std::filesystem::absolute(filepath, ec); ec) {
            return false;
        }
    }
    if(!filepath.has_extension()) {
        return false;
    }
    if(filepath.extension() == ".xml") {
        return ExportAsXml(map, filepath);
    }
    if(filepath.extension() == ".tmx") {
        return ExportAsTmx(map, filepath);
    }
    if(filepath.extension() == ".map") {
        return ExportAsBin(map, filepath);
    }
    return false;
}

bool MapEditor::ExportAsXml(const Map& map, const std::filesystem::path& filepath) const noexcept {
    if(map._xml_doc) {
        if(const auto xml_result = map._xml_doc->SaveFile(filepath.string().c_str()); xml_result == tinyxml2::XML_SUCCESS) {
            return true;
        }
    }
    return false;
}

bool MapEditor::ExportAsTmx(const Map& /*map*/, const std::filesystem::path& /*filepath*/) const noexcept {
    return false;
}

bool MapEditor::ExportAsBin(const Map& /*map*/, const std::filesystem::path& /*filepath*/) const noexcept {
    return false;
}

bool MapEditor::DeserializeMap(Map& map, std::filesystem::path filepath) noexcept {
    if(!std::filesystem::exists(filepath)) {
        return false;
    }
    {
        std::error_code ec{};
        if(filepath = std::filesystem::canonical(filepath, ec); ec) {
            return false;
        }
    }
    if(!filepath.has_extension()) {
        return false;
    }
    if(filepath.extension() == ".xml") {
        return ImportAsXml(map, filepath);
    }
    if(filepath.extension() == ".tmx") {
        return ImportAsTmx(map, filepath);
    }
    if(filepath.extension() == ".map") {
        return ImportAsBin(map, filepath);
    }
    return true;
}

bool MapEditor::ImportAsXml(Map& map, const std::filesystem::path& filepath) noexcept {

    if(map._xml_doc) {
        if(!FileUtils::IsSafeReadPath(filepath)) {
            return false;
        }
        if(std::filesystem::exists(filepath) && filepath.has_extension() && filepath.extension() == ".xml") {
            if(const auto xml_result = map._xml_doc->LoadFile(filepath.string().c_str()); xml_result == tinyxml2::XML_SUCCESS) {
                return true;
            }
        }
    }
    return false;
}

bool MapEditor::ImportAsTmx(Map& map, const std::filesystem::path& filepath) noexcept {
    if(!FileUtils::IsSafeReadPath(filepath)) {
        return false;
    }
    if(map._xml_doc) {
        if(tinyxml2::XML_SUCCESS == map._xml_doc->LoadFile(filepath.string().c_str())) {
            auto* xml_root = map._xml_doc->RootElement();
            map.LoadFromTmx(*xml_root);
        }
    }
    return false;
}

bool MapEditor::ImportAsBin(Map& /*map*/, const std::filesystem::path& /*filepath*/) noexcept {
    return false;
}
