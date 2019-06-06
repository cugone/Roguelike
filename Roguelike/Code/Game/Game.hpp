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

    void LoadMaps();

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render() const;
    void EndFrame();

protected:
private:
    void HandleDebugInput(Camera2D &base_camera);
    void HandlePlayerInput(Camera2D &base_camera);

    void ShowDebugUI();
    void ShowBoundsColoringUI();
    void ShowTileInspectorUI();

    std::unique_ptr<Map> _map{nullptr};
    mutable Camera2D _ui_camera{};
    Rgba _grid_color{Rgba::Red};
    float _cam_speed = 1.0f;
    float _max_shake_angle = 0.0f;
    float _max_shake_x = 0.0f;
    float _max_shake_y = 0.0f;
    bool _show_grid = false;
    bool _show_debug_window = false;
    bool _show_world_bounds = false;

    friend class Map;
    friend class Layer;
};

