#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Math/IntVector2.hpp"

#include "Engine/Renderer/Camera2D.hpp"
#include "Engine/Renderer/FrameBuffer.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"

class MapEditor {
public:
    MapEditor() noexcept = default;
    explicit MapEditor(IntVector2 dimensions = IntVector2{min_map_width, min_map_height}) noexcept;
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
    std::shared_ptr<FrameBuffer> m_viewport_fb{FrameBuffer::Create(FrameBufferDesc{})};
    uint32_t m_ViewportWidth{1600u};
    uint32_t m_ViewportHeight{900u};
    OrthographicCameraController m_editorCamera{};
    bool m_hasUnsavedChanges{ false };
    mutable Camera2D m_uiCamera;
};
