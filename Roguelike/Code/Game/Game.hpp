#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Renderer/Camera2D.hpp"

#include "Game/Map.hpp"

#include <memory>

struct fullscreen_cb_t {
    int effectIndex = -1;
    float fadePercent = 0.0f;
    float greyscaleBrightness = 1.2f;
    float shadowmask_alpha = 0.5f;
    Vector4 fadeColor{};
    IntVector2 resolution{};
    Vector2 hardness{ -8.0f, -3.0f };
    Vector2 mask{ 0.5f, 1.5f };
    Vector2 warp{ 1.0f / 32.0f, 1.0f / 24.0f };
    Vector2 res{};
    Vector2 padding{};
};

enum class FullscreenEffect {
    None = -1
    ,FadeIn
    ,FadeOut
    ,Scanlines
    ,Greyscale
    ,Sepia
};

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

    void HandlePlayerInput(Camera2D& base_camera);

    void ShowDebugUI();

    std::vector<Tile*> DebugGetTilesFromMouse();

    void ShowTileDebuggerUI();
    void ShowEntityDebuggerUI();
    void ShowEffectsDebuggerUI();

    void ShowEffectsUI();
    void ShowBoundsColoringUI();
    void ShowTileInspectorUI();

    void ShowEntityInspectorUI();
    void ShowEntityInspectorEntityColumnUI(const Entity* cur_entity, const AnimatedSprite* cur_sprite);
    void ShowEntityInspectorInventoryColumnUI(const Entity* cur_entity);

    void LoadMaps();

    void UpdateFullscreenEffect(const FullscreenEffect& effect);
    bool DoFadeIn(const Rgba& color, TimeUtils::FPSeconds fadeTime);
    bool DoFadeOut(const Rgba& color, TimeUtils::FPSeconds fadeTime);
    void DoScanlines();
    void DoGreyscale(float brightnessPower = 2.4f);
    void DoSepia();
    void StopFullscreenEffect();

    std::unique_ptr<Map> _map{nullptr};
    mutable Camera2D _ui_camera{};
    Rgba _grid_color{Rgba::Red};
    std::vector<Tile*> _debug_inspected_tiles{};
    Entity* _debug_inspected_entity = nullptr;
    float _cam_speed = 1.0f;
    float _max_shake_angle = 0.0f;
    float _max_shake_x = 0.0f;
    float _max_shake_y = 0.0f;
    float _debug_fadeInTime = 1.0f;
    float _debug_fadeOutTime = 1.0f;
    bool _debug_has_picked_entity_with_click = false;
    bool _debug_has_picked_tile_with_click = false;
    bool _player_requested_wait = false;
    bool _show_grid = false;
    bool _show_debug_window = false;
    bool _show_world_bounds = false;
    bool _show_tile_debugger = false;
    bool _show_effects_debugger = false;
    bool _show_entity_debugger = false;
    bool _show_all_entities = false;
    std::unique_ptr<class ConstantBuffer> _fullscreen_cb = nullptr;
    fullscreen_cb_t _fullscreen_data = fullscreen_cb_t{};
    FullscreenEffect _current_fs_effect = FullscreenEffect::None;
    Rgba _fadeIn_color = Rgba::Black;
    Rgba _fadeOut_color = Rgba::Black;
    TimeUtils::FPSeconds _fadeInTime{};
    TimeUtils::FPSeconds _fadeOutTime{};

    friend class Map;
    friend class Layer;
};

