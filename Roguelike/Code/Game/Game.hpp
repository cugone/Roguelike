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

protected:
private:
    void HandleDebugInput(Camera2D& base_camera);
    void HandleDebugKeyboardInput(Camera2D& base_camera);
    void HandleDebugMouseInput(Camera2D& base_camera);

    void HandlePlayerInput();

    void ShowDebugUI();

    std::vector<Tile*> DebugGetTilesFromMouse();

    void ShowTileDebuggerUI();
    void ShowEntityDebuggerUI();

    void ShowBoundsColoringUI();
    void ShowTileInspectorUI();

    void ShowEntityInspectorUI();
    void ShowEntityInspectorEntityColumnUI(const Entity* cur_entity, const AnimatedSprite* cur_sprite);
    void ShowEntityInspectorInventoryColumnUI(const Entity* cur_entity);

    void LoadMaps();

    std::unique_ptr<Map> _map{nullptr};
    mutable Camera2D _ui_camera{};
    Rgba _grid_color{Rgba::Red};
    bool _debug_has_picked_tile_with_click = false;
    bool _debug_has_picked_entity_with_click = false;
    std::vector<Tile*> _debug_inspected_tiles{};
    Entity* _debug_inspected_entity = nullptr;
    float _cam_speed = 1.0f;
    float _max_shake_angle = 0.0f;
    float _max_shake_x = 0.0f;
    float _max_shake_y = 0.0f;
    bool _show_grid = false;
    bool _show_debug_window = false;
    bool _show_world_bounds = false;
    bool _show_tile_debugger = false;
    bool _show_entity_debugger = false;
    bool _show_all_entities = false;

    friend class Map;
    friend class Layer;
};

