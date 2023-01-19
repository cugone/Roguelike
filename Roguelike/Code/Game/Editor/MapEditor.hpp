#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/FrameBuffer.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"

#include <filesystem>

class MapEditor {
public:
    MapEditor() noexcept = default;
    explicit MapEditor(IntVector2 dimensions = IntVector2{min_map_width, min_map_height}) noexcept;
    explicit MapEditor(const std::filesystem::path& mapPath) noexcept;
    void BeginFrame_Editor() noexcept;
    void Update_Editor(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render_Editor() const noexcept;
    void EndFrame_Editor() noexcept;

    bool SerializeMap(const Map& map, std::filesystem::path filepath) const noexcept;
    bool DeserializeMap(Map& map, std::filesystem::path filepath) noexcept;

protected:
private:
    void ShowMainMenu([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowViewport([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowProperties([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;

    void DoSave() noexcept;
    void DoSaveAs() noexcept;

    bool ExportAsXml(const Map& map, const std::filesystem::path& filepath) const noexcept;
    bool ExportAsTmx(const Map& map, const std::filesystem::path& filepath) const noexcept;
    bool ExportAsBin(const Map& map, const std::filesystem::path& filepath) const noexcept;


    bool ImportAsXml(Map& map, const std::filesystem::path& filepath) noexcept;
    bool ImportAsTmx(Map& map, const std::filesystem::path& filepath) noexcept;
    bool ImportAsBin(Map& map, const std::filesystem::path& filepath) noexcept;


    Map m_editorMap;
    std::filesystem::path m_map_path{};
    std::shared_ptr<FrameBuffer> m_viewport_fb{FrameBuffer::Create(FrameBufferDesc{})};
    uint32_t m_ViewportWidth{1600u};
    uint32_t m_ViewportHeight{900u};
    OrthographicCameraController m_editorCamera{};
    bool m_hasUnsavedChanges{ false };
    mutable Camera2D m_uiCamera;
};
