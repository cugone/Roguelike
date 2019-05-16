#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Renderer/Camera2D.hpp"

#include "Game/Map.hpp"

#include <memory>


class Game {
public:
    Game() = default;
    Game(const Game& other) = default;
    Game(Game&& other) = default;
    Game& operator=(const Game& other) = default;
    Game& operator=(Game&& other) = default;
    ~Game() = default;

    void Initialize();
    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render() const;
    void EndFrame();

    Camera2D& GetCamera() const;

protected:
private:
    void ShowDebugUI();

    bool _debug = false;
    bool _show_grid = false;
    bool _show_debug_window = false;
    std::unique_ptr<Map> _map{nullptr};
    float _cam_speed = 1.0f;
    mutable Camera2D _ui_camera{};
};
