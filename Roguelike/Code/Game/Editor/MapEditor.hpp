#pragma once

#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/Camera2D.hpp"

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
    Map m_editorMap;
    OrthographicCameraController m_cameraController;
    mutable Camera2D m_uiCamera;
};
