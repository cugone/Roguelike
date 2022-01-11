#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/FrameBuffer.hpp"

#include "Game/Map.hpp"

class MapEditor {
public:
    explicit MapEditor(const std::filesystem::path& mapPath) noexcept;
    void BeginFrame_Editor() noexcept;
    void Update_Editor(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render_Editor() const noexcept;
    void EndFrame_Editor() noexcept;

    bool SerializeMap(const Map& map) const noexcept;
    bool DeserializeMap(Map& map) noexcept;

protected:
private:
    void ShowMainMenu([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowViewport([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;
    void ShowProperties([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept;

    Map m_editorMap;
    std::shared_ptr<FrameBuffer> m_viewport_fb{};
    uint32_t m_ViewportWidth{1600u};
    uint32_t m_ViewportHeight{900u};
    OrthographicCameraController m_cameraController;
    bool m_hasUnsavedChanges{ false };
    mutable Camera2D m_uiCamera;
};
