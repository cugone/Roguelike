#pragma once

#include "Engine/Core/TimeUtils.hpp"

class MapEditor {
public:
    void BeginFrame_Editor() noexcept;
    void Update_Editor(TimeUtils::FPSeconds deltaSeconds) noexcept;
    void Render_Editor() const noexcept;
    void EndFrame_Editor() noexcept;
protected:
private:
    
};
