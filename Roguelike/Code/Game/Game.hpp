#pragma once

#include "Engine/Core/Console.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Game/Cursor.hpp"
#include "Game/Map.hpp"
#include "Game/GameCommon.hpp"

#include <memory>

class KerningFont;

struct fullscreen_cb_t {
    int effectIndex = -1;
    float fadePercent = 0.0f;
    float lumosityBrightness = 1.2f;
    float gradiantRadius = 0.5f;
    Vector4 fadeColor{};
    Vector4 gradiantColor = Rgba::White.GetRgbaAsFloats();
};

enum class FullscreenEffect {
    None = -1
    ,FadeIn
    ,FadeOut
    ,Lumosity
    ,Sepia
    ,CircularGradient
};

enum class GameState {
    Title
    ,Loading
    ,Main
};

class Game {
public:
    Game() = default;
    Game(const Game& other) = default;
    Game(Game&& other) = default;
    Game& operator=(const Game& other) = default;
    Game& operator=(Game&& other) = default;
    ~Game() noexcept;

    void Initialize();
    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render() const;
    void EndFrame();

    bool HasCursor(const std::string& name) const noexcept;
    void SetCurrentCursorByName(const std::string& name) noexcept;

    KerningFont* ingamefont{};
    Cursor* current_cursor{};
protected:
private:

    void OnEnter_Title();
    void OnEnter_Loading();
    void OnEnter_Main();

    void OnExit_Title();
    void OnExit_Loading();
    void OnExit_Main();

    void BeginFrame_Title();
    void BeginFrame_Loading();
    void BeginFrame_Main();

    void Update_Title(TimeUtils::FPSeconds deltaSeconds);
    void Update_Loading(TimeUtils::FPSeconds deltaSeconds);
    void Update_Main(TimeUtils::FPSeconds deltaSeconds);

    void Render_Title() const;
    void Render_Loading() const;
    void Render_Main() const;
    
    void EndFrame_Title();
    void EndFrame_Loading();
    void EndFrame_Main();

    void ChangeGameState(const GameState& newState);
    void OnEnterState(const GameState& state);
    void OnExitState(const GameState& state);

    void CreateFullscreenConstantBuffer();

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

    void LoadUI();
    void LoadMaps();
    void LoadEntities();
    void LoadItems();
    void LoadCursorsFromFile(const std::filesystem::path& src);
    void LoadCursorDefinitionsFromFile(const std::filesystem::path& src);

    void LoadEntitiesFromFile(const std::filesystem::path& src);
    void LoadEntityDefinitionsFromFile(const std::filesystem::path& src);
    void LoadItemsFromFile(const std::filesystem::path& src);

    void UpdateFullscreenEffect(const FullscreenEffect& effect);
    bool DoFadeIn(const Rgba& color, TimeUtils::FPSeconds fadeTime);
    bool DoFadeOut(const Rgba& color, TimeUtils::FPSeconds fadeTime);
    void DoLumosity(float brightnessPower = 2.4f);
    void DoCircularGradient(float radius, const Rgba& color);
    void DoSepia();
    void StopFullscreenEffect();

    void RegisterCommands();
    void UnRegisterCommands();

    std::unique_ptr<Map> _map{nullptr};
    mutable Camera2D _ui_camera{};
    Rgba _grid_color{Rgba::Red};
    Rgba _debug_gradientColor{Rgba::White};
    std::vector<Tile*> _debug_inspected_tiles{};
    Entity* _debug_inspected_entity = nullptr;
    std::shared_ptr<SpriteSheet> _cursor_sheet{};
    std::shared_ptr<SpriteSheet> _entity_sheet{};
    std::shared_ptr<SpriteSheet> _item_sheet{};

    std::map<std::string, Cursor> _cursors{};

    float _cam_speed = 1.0f;
    float _max_shake_angle = 0.0f;
    float _max_shake_x = 0.0f;
    float _max_shake_y = 0.0f;
    float _debug_fadeInTime = 1.0f;
    float _debug_fadeOutTime = 1.0f;
    float _debug_gradientRadius = 0.5f;
    float _text_alpha = 1.0f;
    bool _debug_has_picked_entity_with_click = false;
    bool _debug_has_picked_tile_with_click = false;
    bool _player_requested_wait = false;
    bool _debug_render = false;
    bool _show_grid = false;
    bool _show_debug_window = false;
    bool _show_world_bounds = false;
    bool _show_tile_debugger = false;
    bool _show_effects_debugger = false;
    bool _show_entity_debugger = false;
    bool _show_all_entities = false;
    bool _done_loading = false;
    std::unique_ptr<class ConstantBuffer> _fullscreen_cb = nullptr;
    fullscreen_cb_t _fullscreen_data = fullscreen_cb_t{};
    FullscreenEffect _current_fs_effect = FullscreenEffect::None;
    Rgba _fadeIn_color = Rgba::Black;
    Rgba _fadeOut_color = Rgba::Black;
    TimeUtils::FPSeconds _fadeInTime{};
    TimeUtils::FPSeconds _fadeOutTime{};
    Console::CommandList _consoleCommands;
    GameState _currentGameState = GameState::Title;
    GameState _nextGameState = GameState::Title;

    friend class Map;
    friend class Layer;
    friend class Tile;
};

