#pragma once

#include "Engine/Core/Console.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Label.hpp"
#include "Engine/UI/Panel.hpp"
#include "Engine/UI/Widget.hpp"

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
    ,SquareBlur
};

enum class GameState {
    Title
    ,Loading
    ,Main
};

enum class CursorId {
    First_
    ,Green_Box = First_
    ,Question
    ,Red_Crosshair_Box
    ,Yellow_Corner_Box
    ,Last_
    ,Max = Last_
};

class Game {
public:
    Game();
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
    bool HasCursor(CursorId id) const noexcept;
    void SetCurrentCursorByName(const std::string& name) noexcept;
    void SetCurrentCursorById(CursorId id) noexcept;

    KerningFont* ingamefont{};
    Cursor* current_cursor{};
    CursorId current_cursorId{};
    mutable Camera2D ui_camera{};
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

    void HandleDebugInput();
    void HandleDebugKeyboardInput();
    void HandleDebugMouseInput();

    void HandlePlayerInput();
    void HandlePlayerKeyboardInput();
    void HandlePlayerMouseInput();
    void HandlePlayerControllerInput();

    void ZoomOut();
    void ZoomIn();

#ifdef PROFILE_BUILD
    void ShowDebugUI();

    std::vector<Tile*> DebugGetTilesFromMouse();
    std::vector<Tile*> DebugGetTilesFromCursor();

    void ShowTileDebuggerUI();
    void ShowEntityDebuggerUI();
    void ShowFeatureDebuggerUI();
    void ShowEffectsDebuggerUI();

    void ShowEffectsUI();
    void ShowFrameInspectorUI();
    void ShowWorldInspectorUI();
    void ShowTileInspectorUI();

    void ShowEntityInspectorUI();
    void ShowEntityInspectorEntityColumnUI(const Entity* cur_entity, const AnimatedSprite* cur_sprite);
    void ShowEntityInspectorInventoryColumnUI(const Entity* cur_entity);

    void ShowFeatureInspectorUI();
#endif

    const bool IsDebugWindowOpen() const noexcept;

    void LoadUI();
    void LoadMaps();
    void LoadEntities();
    void LoadItems();
    void LoadCursorsFromFile(const std::filesystem::path& src);
    void LoadCursorDefinitionsFromFile(const std::filesystem::path& src);

    void LoadEntitiesFromFile(const std::filesystem::path& src);
    void LoadEntityDefinitionsFromFile(const std::filesystem::path& src);
    void LoadItemsFromFile(const std::filesystem::path& src);

    void RequestScreenShot() const noexcept;
    void UpdateFullscreenEffect(const FullscreenEffect& effect);
    bool DoFadeIn(const Rgba& color, TimeUtils::FPSeconds fadeTime);
    bool DoFadeOut(const Rgba& color, TimeUtils::FPSeconds fadeTime);
    void DoLumosity(float brightnessPower = 2.4f);
    void DoCircularGradient(float radius, const Rgba& color);
    void DoSepia();
    void DoSquareBlur();
    void StopFullscreenEffect();
    void SetFullscreenEffect(FullscreenEffect effect, const std::function<void()>& onDoneCallback);

    void RegisterCommands();
    void UnRegisterCommands();

    void LoadData(void* user_data);

    std::unique_ptr<Map> _map{nullptr};
    Rgba _grid_color{Rgba::Red};
    Rgba _debug_gradientColor{Rgba::White};
    std::vector<Tile*> _debug_inspected_tiles{};
    Entity* _debug_inspected_entity = nullptr;
    Feature* _debug_inspected_feature = nullptr;
    std::shared_ptr<SpriteSheet> _cursor_sheet{};
    std::shared_ptr<SpriteSheet> _entity_sheet{};
    std::shared_ptr<SpriteSheet> _item_sheet{};
    std::unique_ptr<UI::Widget> _widgetLoading{};
    std::vector<Cursor> _cursors{};
    float _cam_speed = 5.0f;
    float _max_shake_angle = 0.0f;
    float _max_shake_x = 0.0f;
    float _max_shake_y = 0.0f;
    float _debug_fadeInTime = 1.0f;
    float _debug_fadeOutTime = 1.0f;
    float _debug_fadeOutInTime = 1.0f;
    float _debug_gradientRadius = 0.5f;
    float _text_alpha = 1.0f;
    std::unique_ptr<class ConstantBuffer> _fullscreen_cb = nullptr;
    fullscreen_cb_t _fullscreen_data = fullscreen_cb_t{};
    FullscreenEffect _current_fs_effect = FullscreenEffect::None;
    std::function<void()> _fullscreen_callback{};
    Rgba _fadeIn_color = Rgba::Black;
    Rgba _fadeOut_color = Rgba::Black;
    TimeUtils::FPSeconds _fadeInTime{1.0f};
    TimeUtils::FPSeconds _fadeOutTime{1.0f};
    Console::CommandList _consoleCommands;
    GameState _currentGameState = GameState::Title;
    GameState _nextGameState = GameState::Title;
    std::condition_variable _loading_signal{};
    uint8_t _debug_has_picked_entity_with_click : 1;
    uint8_t _debug_has_picked_feature_with_click : 1;
    uint8_t _debug_has_picked_tile_with_click : 1;
    uint8_t _player_requested_wait : 1;
    uint8_t _debug_render : 1;
    uint8_t _show_grid : 1;
    uint8_t _show_debug_window : 1;
    uint8_t _show_raycasts : 1;
    uint8_t _show_world_bounds : 1;
    uint8_t _show_camera_bounds : 1;
    uint8_t _show_tile_debugger : 1;
    uint8_t _show_effects_debugger : 1;
    uint8_t _show_entity_debugger : 1;
    uint8_t _show_feature_debugger : 1;
    uint8_t _show_all_entities : 1;
    uint8_t _show_camera : 1;
    uint8_t _show_room_bounds : 1;
    uint8_t _show_tile_texture : 1;
    uint8_t _done_loading : 1;
    uint8_t _reset_loading_flag : 1;
    uint8_t _skip_frame : 1;
    friend class Map;
    friend class Layer;
    friend class Tile;
};

