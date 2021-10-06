#pragma once

#include "Engine/Core/Console.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Engine/Game/GameBase.hpp"
#include "Engine/Game/GameSettings.hpp"

#include "Engine/Renderer/Camera2D.hpp"

#include "Engine/UI/UICanvas.hpp"
#include "Engine/UI/UILabel.hpp"
#include "Engine/UI/UIPanel.hpp"
#include "Engine/UI/UIWidget.hpp"

#include "Game/Adventure.hpp"
#include "Game/Cursor.hpp"
#include "Game/Map.hpp"
#include "Game/GameCommon.hpp"

#include <memory>

class KerningFont;
class AnimatedSprite;
class MapEditor;

struct fullscreen_cb_t {
    int effectIndex = -1;
    float fadePercent = 0.0f;
    float lumosityBrightness = 1.2f;
    float gradiantRadius = 0.5f;
    Vector4 fadeColor{};
    Vector4 gradiantColor{1.0f, 1.0f, 1.0f, 1.0f};
};

enum class FullscreenEffect {
    None = -1
    , FadeIn
    , FadeOut
    , Lumosity
    , Sepia
    , CircularGradient
    , SquareBlur
};

enum class GameState {
    Title
    , Loading
    , Main
    , Editor
    , Editor_Main
};

enum class CursorId {
    First_
    , Yellow_Corner_Box = First_
    , Green_Box
    , Red_Crosshair_Box
    , Question
    , Last_
    , Max = Last_
};

class GameOptions : public GameSettings {
public:
    GameOptions() noexcept = default;
    GameOptions(const GameOptions& other) noexcept = default;
    GameOptions(GameOptions&& other) noexcept = default;
    virtual ~GameOptions() noexcept = default;
    GameOptions& operator=(const GameOptions& rhs) noexcept = default;
    GameOptions& operator=(GameOptions&& rhs) noexcept = default;

    virtual void SaveToConfig(Config& config) noexcept override;
    virtual void SetToDefault() noexcept override;

    void SetSoundVolume(uint8_t newSoundVolume) noexcept;
    uint8_t GetSoundVolume() const noexcept;
    uint8_t DefaultSoundVolume() const noexcept;

    void SetMusicVolume(uint8_t newMusicVolume) noexcept;
    uint8_t GetMusicVolume() const noexcept;
    uint8_t DefaultMusicVolume() const noexcept;

    void SetCameraShakeStrength(float newCameraShakeStrength) noexcept;
    float GetCameraShakeStrength() const noexcept;
    float DefaultCameraShakeStrength() const noexcept;

    void SetCameraSpeed(float newCameraSpeed) noexcept;
    float GetCameraSpeed() const noexcept;
    float DefaultCameraSpeed() const noexcept;

    float GetMaxShakeOffsetHorizontal() const noexcept;
    float GetMaxShakeOffsetVertical() const noexcept;
    float GetMaxShakeAngle() const noexcept;

protected:

    uint8_t _soundVolume{5};
    uint8_t _defaultSoundVolume{5};
    uint8_t _musicVolume{5};
    uint8_t _defaultMusicVolume{5};
    float _cameraShakeStrength{1.0f};
    float _defaultCameraShakeStrength{1.0f};
    float _camSpeed{5.0f};
    float _defaultCamSpeed{5.0f};

private:
    float _maxShakeOffsetHorizontal{50.0f};
    float _maxShakeOffsetVertical{50.0f};
    float _maxShakeAngle{10.0f};
};


class Game : public GameBase {
public:
    Game();
    Game(const Game& other) = default;
    Game(Game&& other) = default;
    Game& operator=(const Game& other) = default;
    Game& operator=(Game&& other) = default;
    ~Game() noexcept;

    void Initialize() noexcept override;
    void BeginFrame() noexcept override;
    void Update([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) noexcept override;
    void Render() const noexcept override;
    void EndFrame() noexcept override;

    bool HasCursor(const std::string& name) const noexcept;
    bool HasCursor(CursorId id) const noexcept;
    void SetCurrentCursorByName(const std::string& name) noexcept;
    void SetCurrentCursorById(CursorId id) noexcept;

    KerningFont* ingamefont{};
    Cursor* current_cursor{};
    CursorId current_cursorId{};
    mutable Camera2D ui_camera{};
    GameOptions gameOptions{};
    bool IsDebugging() const noexcept;
protected:
private:

    void OnEnter_Title();
    void OnEnter_Loading();
    void OnEnter_Main();
    void OnEnter_Editor();
    void OnEnter_EditorMain();

    void OnExit_Title();
    void OnExit_Loading();
    void OnExit_Main();
    void OnExit_Editor();
    void OnExit_EditorMain();

    void BeginFrame_Title();
    void BeginFrame_Loading();
    void BeginFrame_Main();
    void BeginFrame_Editor();
    void BeginFrame_EditorMain();

    void Update_Title(TimeUtils::FPSeconds deltaSeconds);
    void Update_Loading(TimeUtils::FPSeconds deltaSeconds);
    void Update_Main(TimeUtils::FPSeconds deltaSeconds);
    void Update_Editor(TimeUtils::FPSeconds deltaSeconds);
    void Update_EditorMain(TimeUtils::FPSeconds deltaSeconds);

    void Render_Title() const;
    void Render_Loading() const;
    void Render_Main() const;
    void Render_Editor() const;
    void Render_EditorMain() const;

    void EndFrame_Title();
    void EndFrame_Loading();
    void EndFrame_Main();
    void EndFrame_Editor();
    void EndFrame_EditorMain();

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

    std::optional<std::vector<Tile*>> DebugGetTilesFromCursor();

    void ShowTileDebuggerUI();
    void ShowEntityDebuggerUI();
    void ShowFeatureDebuggerUI();
    void ShowEffectsDebuggerUI();

    void ShowEffectsUI();
    void ShowFrameInspectorUI();
    void ShowWorldInspectorUI();
    void ShowTileInspectorUI();
    void ShowTileInspectorTableUI(const std::vector<Tile*>& tiles, const uint8_t tiles_per_row, const uint8_t tiles_per_col);
    void ShowInspectedElementImageUI(const  AnimatedSprite* cur_sprite, const Vector2& dims, const AABB2& tex_coords) noexcept;
    void ShowTileInspectorStatsTableUI(const  TileDefinition* cur_def, const  Tile* cur_tile);
    void ShowEntityInspectorUI();
    void ShowEntityInspectorInventoryManipulatorUI(Entity* const cur_entity);
    void ShowEntityInspectorEntityColumnUI(const Entity* cur_entity, const AnimatedSprite* cur_sprite);
    void ShowEntityInspectorInventoryColumnUI(Entity* const cur_entity);
    void ShowFeatureInspectorUI();
    void ShowEntityInspectorImageUI(const AnimatedSprite* cur_sprite, const Entity* cur_entity);
    void ShowInspectedActorEquipmentExceptImageUI(const AnimatedSprite* cur_sprite, const Entity* cur_entity, const EquipSlot& skip_equip_slot = EquipSlot::None) noexcept;
    void ShowInspectedActorEquipmentOnlyImageUI(const AnimatedSprite* cur_sprite, const Entity* cur_entity, const EquipSlot& equip_slot) noexcept;

#endif

    const bool IsDebugWindowOpen() const noexcept;

    void LoadUI();
    void LoadMaps();
    void LoadEntities();
    void LoadItems();
    void LoadCursorsFromFile(const std::filesystem::path& src);
    void LoadCursorDefinitionsFromFile(const std::filesystem::path& src);

    void ThrowIfSourceFileNotFound(const std::filesystem::path& src);
    void LoadEntitiesFromFile(const std::filesystem::path& src);
    XMLElement* ThrowIfSourceFileNotLoaded(tinyxml2::XMLDocument& doc, const std::filesystem::path& src);
    void LoadEntityDefinitionsFromFile(const std::filesystem::path& src);
    void LoadItemsFromFile(const std::filesystem::path& src);

    void RequestScreenShot() const noexcept;
    void UpdateFullscreenEffect(const FullscreenEffect& effect);
    bool DoFade(const Rgba& color, TimeUtils::FPSeconds fadeTime, FullscreenEffect fadeType);
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

    void MapEntered() noexcept;
    void MapExited() noexcept;

    std::unique_ptr<Adventure> _adventure{nullptr};
    Rgba _grid_color{Rgba::Red};
    Rgba _debug_gradientColor{Rgba::White};
    std::vector<Tile*> _debug_inspected_tiles{};
    Entity* _debug_inspected_entity = nullptr;
    Feature* _debug_inspected_feature = nullptr;
    std::shared_ptr<SpriteSheet> _cursor_sheet{};
    std::shared_ptr<SpriteSheet> _entity_sheet{};
    std::shared_ptr<SpriteSheet> _item_sheet{};
    std::unique_ptr<UIWidget> _widgetLoading{};
    std::vector<Cursor> _cursors{};
    float _debug_fadeInTime = 1.0f;
    float _debug_fadeOutTime = 1.0f;
    float _debug_fadeOutInTime = 1.0f;
    float _debug_gradientRadius = 0.5f;
    float _text_alpha = 1.0f;
    std::unique_ptr<class ConstantBuffer> _fullscreen_cb = nullptr;
    fullscreen_cb_t _fullscreen_data = fullscreen_cb_t{};
    FullscreenEffect _current_fs_effect = FullscreenEffect::None;
    std::function<void()> _fullscreen_callback{};
    Event<> OnMapEnter;
    Event<> OnMapExit;
    Rgba _fadeIn_color = Rgba::Black;
    Rgba _fadeOut_color = Rgba::Black;
    TimeUtils::FPSeconds _fadeInTime{1.0f};
    TimeUtils::FPSeconds _fadeOutTime{1.0f};
    Console::CommandList _consoleCommands;
    GameState _currentGameState = GameState::Title;
    GameState _nextGameState = GameState::Title;
    Stopwatch _filewatcherUpdateRate{1.0f};
    std::thread _filewatcher_worker{};
    std::condition_variable _filewatcher_signal{};
    std::condition_variable _loading_signal{};
    std::filesystem::path m_requested_map_to_load{};
    std::unique_ptr<MapEditor> _editor{};
    uint8_t _player_requested_wait : 1;
    uint8_t _done_loading : 1;
    uint8_t _reset_loading_flag : 1;
    uint8_t _skip_frame : 1;
    uint8_t _menu_id : 1;
#ifdef UI_DEBUG
    uint8_t _debug_has_picked_entity_with_click : 1;
    uint8_t _debug_has_picked_feature_with_click : 1;
    uint8_t _debug_has_picked_tile_with_click : 1;
    uint8_t _debug_render : 1;
    uint8_t _debug_show_grid : 1;
    uint8_t _debug_show_debug_window : 1;
    uint8_t _debug_show_raycasts : 1;
    uint8_t _debug_show_world_bounds : 1;
    uint8_t _debug_show_camera_bounds : 1;
    uint8_t _debug_show_tile_debugger : 1;
    uint8_t _debug_show_entity_debugger : 1;
    uint8_t _debug_show_feature_debugger : 1;
    uint8_t _debug_show_all_entities : 1;
    uint8_t _debug_show_camera : 1;
    uint8_t _debug_show_room_bounds : 1;
#endif
    friend class Map;
    friend class Layer;
    friend class Tile;
};

