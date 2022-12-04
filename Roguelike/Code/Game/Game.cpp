#include "Game/Game.hpp"

#include "Engine/Core/App.hpp"
#include "Engine/Core/ArgumentParser.hpp"
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/Utilities.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Platform/PlatformUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/RHI/RHIOutput.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"
#include "Game/Entity.hpp"
#include "Game/Actor.hpp"
#include "Game/Feature.hpp"
#include "Game/Cursor.hpp"
#include "Game/CursorDefinition.hpp"
#include "Game/EntityDefinition.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/Editor/MapEditor.hpp"
#include "Game/Tile.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Item.hpp"
#include "Game/Inventory.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <string>
#include <utility>

void GameOptions::SaveToConfig(Config& config) noexcept {
    GameSettings::SaveToConfig(config);
    config.SetValue("soundVolume", _soundVolume);
    config.SetValue("musicVolume", _musicVolume);
    config.SetValue("cameraShakeStr", _cameraShakeStrength);
    config.SetValue("cameraSpeed", _camSpeed);
}

void GameOptions::SetToDefault() noexcept {
    _soundVolume = _defaultSoundVolume;
    _musicVolume = _defaultMusicVolume;
    _cameraShakeStrength = _defaultCameraShakeStrength;
    _camSpeed = _defaultCamSpeed;
}

void GameOptions::SetSoundVolume(uint8_t newSoundVolume) noexcept {
    _soundVolume = newSoundVolume;
}

uint8_t GameOptions::GetSoundVolume() const noexcept {
    return _soundVolume;
}

uint8_t GameOptions::DefaultSoundVolume() const noexcept {
    return _defaultSoundVolume;
}

void GameOptions::SetMusicVolume(uint8_t newMusicVolume) noexcept {
    _musicVolume = newMusicVolume;
}

uint8_t GameOptions::GetMusicVolume() const noexcept {
    return _musicVolume;
}

uint8_t GameOptions::DefaultMusicVolume() const noexcept {
    return _defaultMusicVolume;
}

void GameOptions::SetCameraShakeStrength(float newCameraShakeStrength) noexcept {
    _cameraShakeStrength = newCameraShakeStrength;
}

float GameOptions::GetCameraShakeStrength() const noexcept {
    return _cameraShakeStrength;
}

float GameOptions::DefaultCameraShakeStrength() const noexcept {
    return _defaultCameraShakeStrength;
}


void GameOptions::SetCameraSpeed(float newCameraSpeed) noexcept {
    _camSpeed = newCameraSpeed;
}

float GameOptions::GetCameraSpeed() const noexcept {
    return _camSpeed;
}

float GameOptions::DefaultCameraSpeed() const noexcept {
    return _defaultCamSpeed;
}

float GameOptions::GetMaxShakeOffsetHorizontal() const noexcept {
    return _maxShakeOffsetHorizontal;
}

float GameOptions::GetMaxShakeOffsetVertical() const noexcept {
    return _maxShakeOffsetVertical;
}

float GameOptions::GetMaxShakeAngle() const noexcept {
    return _maxShakeAngle;
}


Game::Game()
    : _player_requested_wait{0}
    , _done_loading{0}
    , _reset_loading_flag{0}
    , _skip_frame{0}
    , _menu_id{0}
#ifdef UI_DEBUG
    , _debug_has_picked_entity_with_click{0}
    , _debug_has_picked_feature_with_click{0}
    , _debug_has_picked_tile_with_click{0}
    , _debug_render{0}
    , _debug_show_grid{0}
    , _debug_show_debug_window{0}
    , _debug_show_raycasts{0}
    , _debug_show_world_bounds{0}
    , _debug_show_camera_bounds{0}
    , _debug_show_tile_debugger{0}
    , _debug_show_entity_debugger{0}
    , _debug_show_feature_debugger{0}
    , _debug_show_all_entities{0}
    , _debug_show_camera{0}
    , _debug_show_room_bounds{0}
#endif
{
    /* DO NOTHING */
}

Game::~Game() noexcept {
    _cursors.clear();
    CursorDefinition::ClearCursorRegistry();
    Item::ClearItemRegistry();
    Actor::ClearActorRegistry();
    Feature::ClearFeatureRegistry();
    EntityDefinition::ClearEntityRegistry();
    TileDefinition::ClearTileDefinitions();
}

void Game::Initialize() noexcept {
    if(!g_theConfig->AppendFromFile("Data/Config/options.dat")) {
        g_theFileLogger->LogWarnLine("options file not found at Data/Config/options.dat");
    }
    OnMapExit.Subscribe_method(this, &Game::MapExited);
    OnMapEnter.Subscribe_method(this, &Game::MapEntered);

    _consoleCommands = Console::CommandList(g_theConsole);
    {
        FrameBufferDesc desc{};
        const auto& [width, height] = g_theRenderer->GetOutput()->GetDimensions().GetXY();
        desc.width = width;
        desc.height = height;
        _fullscreen_framebuffer = FrameBuffer::Create(desc);
    }
    CreateFullscreenConstantBuffer();
    g_theRenderer->RegisterMaterialsFromFolder(std::string{"Data/Materials"});
    g_theRenderer->RegisterFontsFromFolder(std::string{"Data/Fonts"});
    ingamefont = g_theRenderer->GetFont("TrebuchetMS32");

    g_theInputSystem->HideMouseCursor();
    //g_theUISystem->RegisterUiWidgetsFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / std::string{"UI"});

}

void Game::CreateFullscreenConstantBuffer() {
    _fullscreen_cb = std::move(g_theRenderer->CreateConstantBuffer(&_fullscreen_data, sizeof(_fullscreen_data)));
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::OnEnter_Title() {
    _adventure.reset(nullptr);
}

void Game::OnEnter_Loading() {
    _done_loading = false;
    _reset_loading_flag = false;
    _skip_frame = true;
    //g_theUISystem->LoadUiWidget("loading");
    //_cnvLoading = std::make_unique<UI::Canvas>(*g_theRenderer);

    //_pnlLoading = _cnvLoading->CreateChild<UI::Panel>();
    //_pnlLoading->SetSize(UI::Metric{UI::Ratio{Vector2::ONE * Vector2{g_theRenderer->GetOutput()->GetDimensions()}}, {}});
    //_pnlLoading->SetPositionRatio(Vector2::ONE * 0.75);

    //_txtLoading = _pnlLoading->CreateChild<UI::Label>(_cnvLoading.get(), ingamefont, std::string{"Loading"});
    //_txtLoading->SetColor(Rgba::White);
    //_txtLoading->SetPositionRatio(Vector2::ONE * 0.5f);

    //_txtLoadingPercentage = _pnlLoading->CreateChild<UI::Label>(_cnvLoading.get(), ingamefont, "0%");
    //_txtLoadingPercentage->SetColor(Rgba::White);
    //_txtLoadingPercentage->SetPosition(Vector4{0.5f, 0.5f, 0.0f, _txtLoading->GetFont()->GetLineHeight()});
}

void Game::OnEnter_Main() {
    RegisterCommands();
    _adventure->CurrentMap()->FocusEntity(_adventure->CurrentMap()->player);
    current_cursor->SetCoords(_adventure->CurrentMap()->player->tile->GetCoords());
    g_theInputSystem->LockMouseToWindowViewport();
}

void Game::OnEnter_Editor() {
    g_theInputSystem->ShowMouseCursor();
}

void Game::OnEnter_EditorMain() {
    g_theInputSystem->ShowMouseCursor();
    if(m_requested_map_to_load.empty()) {
        _editor = std::make_unique<MapEditor>(m_new_dimensions);
    } else {
        _editor = std::make_unique<MapEditor>(m_requested_map_to_load);
    }
    _filewatcherUpdateRate.SetSeconds(TimeUtils::FPSeconds{1.0f});
    _filewatcherUpdateRate.Reset();
}

void Game::OnExit_Title() {
    /* DO NOTHING */
}

void Game::OnExit_Loading() {
    //g_theUISystem->UnloadUiWidget("loading");
    _reset_loading_flag = true;
}

void Game::OnExit_Main() {
    g_theInputSystem->UnlockMouseFromViewport();
    UnRegisterCommands();
    _cursors.clear();
    CursorDefinition::ClearCursorRegistry();
    Item::ClearItemRegistry();
    Actor::ClearActorRegistry();
    Feature::ClearFeatureRegistry();
    EntityDefinition::ClearEntityRegistry();
    TileDefinition::ClearTileDefinitions();
}

void Game::OnExit_Editor() {
    g_theInputSystem->HideMouseCursor();
}

void Game::OnExit_EditorMain() {
    g_theInputSystem->HideMouseCursor();
    _editor.reset(nullptr);
}

void Game::BeginFrame_Title() {
    /* DO NOTHING */
}

void Game::BeginFrame_Loading() {
    /* DO NOTHING */
}

void Game::BeginFrame_Main() {
    _adventure->CurrentMap()->BeginFrame();
}

void Game::BeginFrame_Editor() {
    /* DO NOTHING */
}

void Game::BeginFrame_EditorMain() {
    _editor->BeginFrame_Editor();
}

static const auto MenuId_Start = uint8_t{0};
static const auto MenuId_Editor = uint8_t{1};
static const auto MenuId_Exit = uint8_t{2};

void Game::Update_Title(TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        auto* app = ServiceLocator::get<IAppService>();
        app->SetIsQuitting(true);
        return;
    }
    const auto down = []() {
        const auto keys = g_theInputSystem->WasKeyJustPressed(KeyCode::Down) || g_theInputSystem->WasKeyJustPressed(KeyCode::S);
        if(const auto controller = g_theInputSystem->GetXboxController(0); controller.IsConnected()) {
            if (const auto buttons = controller.WasButtonJustPressed(XboxController::Button::Down)) {
                return buttons;
            }
        }
        return keys;
    }();
    const auto up = []() {
        const auto keys = g_theInputSystem->WasKeyJustPressed(KeyCode::Up) || g_theInputSystem->WasKeyJustPressed(KeyCode::W);
        if(const auto controller = g_theInputSystem->GetXboxController(0); controller.IsConnected()) {
            if (const auto buttons = controller.WasButtonJustPressed(XboxController::Button::Up)) {
                return buttons;
            }
        }
        return keys;
    }();
    const auto select = []() {
        const auto keys = g_theInputSystem->WasKeyJustPressed(KeyCode::Enter);
        if(const auto controller = g_theInputSystem->GetXboxController(0); controller.IsConnected()) {
            if (const auto buttons = controller.WasButtonJustPressed(XboxController::Button::Start)) {
                return buttons;
            }
        }
        return keys;
    }();
    const auto GetSelectedMenuId = [&]()-> void {
        if(up) {
            _menu_id = (std::min)(--_menu_id, MenuId_Start);
        } else if(down) {
            _menu_id = (std::min)(++_menu_id, MenuId_Exit);
        }
    };
    GetSelectedMenuId();
    if(select) {
        switch(_menu_id) {
        case MenuId_Start:
        {
            ChangeGameState(GameState::Loading);
            break;
        }
        case MenuId_Editor:
        {
            ChangeGameState(GameState::Editor);
            break;
        }
        case MenuId_Exit:
        {
            auto* app = ServiceLocator::get<IAppService>();
            app->SetIsQuitting(true);
            break;
        }
        default:
            break;
        }
    }
}

void Game::Update_Loading(TimeUtils::FPSeconds /*deltaSeconds*/) {
    static Stopwatch text_blinker{TimeUtils::FPSeconds{0.33f}};
    if(text_blinker.CheckAndReset()) {
        _text_alpha = 1.0f - _text_alpha;
        _text_alpha = std::clamp(_text_alpha, 0.0f, 1.0f);
    }
    if(_done_loading && g_theInputSystem->WasAnyKeyPressed()) {
        _reset_loading_flag = true;
        ChangeGameState(GameState::Main);
    }
}

void Game::Update_Main(TimeUtils::FPSeconds deltaSeconds) {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        ChangeGameState(GameState::Title);
        return;
    }
    auto* app = ServiceLocator::get<IAppService>();
    if(app->LostFocus()) {
        deltaSeconds = TimeUtils::FPSeconds::zero();
    }
    g_theRenderer->UpdateGameTime(deltaSeconds);
    HandleDebugInput();
    HandlePlayerInput();

    UpdateFullscreenEffect(_current_fs_effect);
    _adventure->CurrentMap()->Update(deltaSeconds);
}

void Game::Update_Editor([[maybe_unused]] TimeUtils::FPSeconds deltaSeconds) {
    static bool showNew = false;
    static std::string mapPath{};
    if(ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New...", "Ctrl+N")) {
                showNew = true;
            }
            if(ImGui::MenuItem("Open...", "Ctrl+O")) {
                if (const auto ofdResult = FileDialogs::OpenFile("Map file (*.xml)\0*.xml\0\0"); !ofdResult.empty()) {
                    mapPath = ofdResult;
                    m_requested_map_to_load = std::filesystem::path{ mapPath };
                    LoadUI();
                    LoadItems();
                    LoadEntities();
                    ChangeGameState(GameState::Editor_Main);
                }
            }
            ImGui::Separator();
            ImGui::MenuItem("Save", "Ctrl+S", nullptr, false);
            ImGui::MenuItem("Save As...", "Ctrl+Shift+S", nullptr, false);
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                ChangeGameState(GameState::Title);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (showNew) {
        if (ImGui::Begin("Map Setup")) {
            static int newWidth = min_map_width;
            static int newHeight = min_map_height;
            static std::string name_str{"Map_Name"};
            ImGui::InputText("Name##MapNameMain", &name_str);
            if(ImGui::SliderInt("Width", &newWidth, min_map_width, max_map_width, "%d", ImGuiSliderFlags_AlwaysClamp)) {
                m_new_dimensions.x = newWidth;
            }
            if(ImGui::SliderInt("Height", &newHeight, min_map_width, max_map_width, "%d", ImGuiSliderFlags_AlwaysClamp)) {
                m_new_dimensions.y = newHeight;
            }
            if (ImGui::Button("OK##OMD")) {
                showNew = false;
                mapPath = std::format("Data/Maps/{}.xml", name_str);
                NewMapOptions opts{};
                opts.name = name_str;
                opts.time = TimeOfDay::Day;
                if(!std::filesystem::exists(mapPath)) {
                    CreateEmptyMapAt(mapPath, opts);
                }
                m_requested_map_to_load = std::filesystem::path{ mapPath };
                LoadUI();
                LoadItems();
                LoadEntities();
                LoadDefaultTileDefinitions();
                ChangeGameState(GameState::Editor_Main);
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel##OMD")) {
                name_str.clear();
                name_str.shrink_to_fit();
                showNew = false;
            }
            ImGui::End();
        }
    }
}

void Game::Update_EditorMain(TimeUtils::FPSeconds deltaSeconds) {
    _editor->Update_Editor(deltaSeconds);
}

void Game::Render_Title() const {

    g_theRenderer->BeginRender();

    g_theRenderer->BeginHUDRender(ui_camera, Vector2::Zero, static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight()));

    static const auto line_height = ingamefont->CalculateTextHeight("X");
    //g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
    //g_theRenderer->DrawFilledCircle2D(Vector2::X_Axis * -20.0f, 10.0f);
    g_theRenderer->DrawTextLine(Matrix4::CreateTranslationMatrix(Vector2{0.0f, line_height * 0.0f}), ingamefont, "RogueLike");

    g_theRenderer->DrawTextLine(Matrix4::CreateTranslationMatrix(Vector2{0.0f, line_height * 2.0f}), ingamefont, "Start", _menu_id == MenuId_Start ? Rgba::Yellow : Rgba::White);
    g_theRenderer->DrawTextLine(Matrix4::CreateTranslationMatrix(Vector2{0.0f, line_height * 4.0f}), ingamefont, "Map Editor", _menu_id == MenuId_Editor ? Rgba::Yellow : Rgba::White);
    g_theRenderer->DrawTextLine(Matrix4::CreateTranslationMatrix(Vector2{0.0f, line_height * 5.0f}), ingamefont, "Exit", _menu_id == MenuId_Exit ? Rgba::Yellow : Rgba::White);

}

void Game::Render_Loading() const {

    g_theRenderer->BeginRender();

    g_theRenderer->BeginHUDRender(ui_camera, Vector2::Zero, static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight()));

    g_theRenderer->SetModelMatrix(Matrix4::I);
    g_theRenderer->DrawTextLine(ingamefont, "LOADING");
    if(_done_loading) {
        static const std::string text = "Press Any Key";
        static const auto text_length = ingamefont->CalculateTextWidth(text);
        g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(Vector2{text_length * -0.25f, ingamefont->GetLineHeight()}));
        const auto color = [&]() { auto result = Rgba::White; result.a = static_cast<unsigned char>(255.0f * _text_alpha); return result; }();
        g_theRenderer->DrawTextLine(ingamefont, text, color);
    }
}

void Game::Render_Main() const {

    g_theRenderer->BeginRenderToBackbuffer(_adventure->CurrentMap()->SkyColor());

    _adventure->CurrentMap()->Render();

#ifdef UI_DEBUG
    if(_debug_render) {
        _adventure->CurrentMap()->DebugRender();
    }
#endif

    auto* app = ServiceLocator::get<IAppService>();
    if(app->LostFocus()) {
        g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        g_theRenderer->DrawQuad2D(Vector2::Zero, Vector2::One, Rgba(0, 0, 0, 128));
    }

    g_theRenderer->BeginHUDRender(ui_camera, Vector2::Zero, static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight()));

    if(app->LostFocus()) {
        const auto w = static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowWidth());
        const auto h = static_cast<float>(GetGameAs<Game>()->GetSettings().GetWindowHeight());
        g_theRenderer->DrawQuad2D(Matrix4::CreateScaleMatrix(Vector2{w, h}), Rgba{0.0f, 0.0f, 0.0f, 0.5f});
        g_theRenderer->SetModelMatrix(Matrix4::I);
        g_theRenderer->DrawTextLine(ingamefont, "PAUSED");
    }
}

void Game::Render_Editor() const {
    g_theRenderer->BeginRenderToBackbuffer(Rgba::NoAlpha);
}

void Game::Render_EditorMain() const {
    _editor->Render_Editor();
}

void Game::EndFrame_Title() {
    /* DO NOTHING */
}

void Game::LoadData(void* /*user_data*/) {
    LoadUI();
    LoadItems();
    LoadEntities();
    LoadMaps();
    _done_loading = true;
}

void Game::CreateEmptyMapAt(const std::filesystem::path& src, const NewMapOptions& opts) noexcept {
    const auto xml_map_prefix = std::format(
R"(<map name="{}" timeOfDay="day">
    <material name="Tile" />
    <tiles src="Data/Definitions/Tiles.xml" />
    <actors>
        <actor name="player" lookAndFeel="human..male" position="[{},{}]" />
    </actors>
    <mapGenerator type="xml">
        <layers>
            <layer>)"
, opts.name
, opts.player_start.x
, opts.player_start.y
);

    const auto xml_map_suffix = std::string {
R"(
            </layer>
        </layers>
    </mapGenerator>
</map>
)"
    };
    using namespace std::literals;
    std::string xml_map_layers{};
    xml_map_layers.reserve(std::size_t{35u + MathUtils::DigitLength(opts.dimensions.x)} * static_cast<std::size_t>(opts.dimensions.y));
    for(int i = 0; i != opts.dimensions.y; ++i) {
        xml_map_layers += std::vformat("\n{0:16}<row glyphs=\"{1:.>{2}}\" />"sv, std::make_format_args("", "", opts.dimensions.x));
    }
    const auto xml = xml_map_prefix + xml_map_layers + xml_map_suffix;
    if(auto success = FileUtils::WriteBufferToFile(xml, src); !success) {
        g_theFileLogger->LogErrorLine(std::format("Could not create map at \"{}\".", src.string()));
    }
}

void Game::SetFullscreenEffect(FullscreenEffect effect, const std::function<void()>& onDoneCallback) {
    _current_fs_effect = effect;
    _fullscreen_callback = onDoneCallback;
}

void Game::RequestScreenShot() const noexcept {
    namespace FS = std::filesystem;
    const auto folder = FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / FS::path{"Screenshots/"};
    const auto screenshot_count = FileUtils::CountFilesInFolders(folder);
    const auto filepath = folder / FS::path{"Screenshot_" + std::to_string(screenshot_count + 1) + ".png"};
    g_theRenderer->RequestScreenShot(filepath);
}

void Game::EndFrame_Loading() {
    if(!_done_loading) {
        if(_skip_frame) {
            _skip_frame = false;
            return;
        }
        Utils::DoOnce(
            [this]() {
                g_theJobSystem->Run(JobType::Generic, [this](void* ud)->void { LoadData(ud); }, nullptr);
            }
        , _reset_loading_flag);
    } else {
        SetCurrentCursorById(CursorId::Yellow_Corner_Box);
        _adventure->CurrentMap()->cameraController.SetPosition(Vector2{_adventure->CurrentMap()->player->GetPosition()} + Vector2{0.5f, 0.5f});
    }
}

void Game::RegisterCommands() {
    g_theConsole->PushCommandList(_consoleCommands);
}

void Game::UnRegisterCommands() {
    g_theConsole->PopCommandList(_consoleCommands);
}

void Game::MapEntered() noexcept {
    static bool requested_enter = false;
    if(_adventure->CurrentMap()->IsPlayerOnEntrance()) {
        requested_enter = true;
    }
    if(requested_enter) {
        _adventure->PreviousMap();
        requested_enter = false;
        //SetFullscreenEffect(FullscreenEffect::FadeOut, [this]() {
        //    _adventure->PreviousMap();
        //    SetFullscreenEffect(FullscreenEffect::FadeIn, []() {
        //        requested_enter = false;
        //    });
        //});
    }
}

void Game::MapExited() noexcept {
    static bool requested_exit = false;
    if(_adventure->CurrentMap()->IsPlayerOnExit()) {
        requested_exit = true;
    }
    if(requested_exit) {
        _adventure->NextMap();
        requested_exit = false;
        //SetFullscreenEffect(FullscreenEffect::FadeOut, [this]() {
        //    _adventure->NextMap();
        //    SetFullscreenEffect(FullscreenEffect::FadeIn, []() {
        //        requested_exit = false;
        //        });
        //});
    }
}

void Game::EndFrame_Main() {
    _adventure->CurrentMap()->EndFrame();
    OnMapExit.Trigger();
    OnMapEnter.Trigger();
}

void Game::EndFrame_Editor() {
    /* DO NOTHING */
}

void Game::EndFrame_EditorMain() {
    _editor->EndFrame_Editor();
}

void Game::ChangeGameState(const GameState& newState) {
    _nextGameState = newState;
}

void Game::OnEnterState(const GameState& state) {
    switch(state) {
    case GameState::Title:         OnEnter_Title();   break;
    case GameState::Loading:       OnEnter_Loading(); break;
    case GameState::Main:          OnEnter_Main();    break;
    case GameState::Editor:        OnEnter_Editor();  break;
    case GameState::Editor_Main:   OnEnter_EditorMain();  break;
    default: ERROR_AND_DIE("ON ENTER UNDEFINED GAME STATE") break;
    }
}

void Game::OnExitState(const GameState& state) {
    switch(state) {
    case GameState::Title:         OnExit_Title();   break;
    case GameState::Loading:       OnExit_Loading(); break;
    case GameState::Main:          OnExit_Main();    break;
    case GameState::Editor:        OnExit_Editor();  break;
    case GameState::Editor_Main:   OnExit_EditorMain();  break;
    default: ERROR_AND_DIE("ON ENTER UNDEFINED GAME STATE") break;
    }
}

const bool Game::IsDebugWindowOpen() const noexcept {
#ifdef PROFILE_BUILD
    return _debug_show_debug_window;
#else
    return false;
#endif
}

void Game::LoadUI() {
    LoadCursorsFromFile(default_ui_src);
}

void Game::LoadAdventureFromFile(const std::filesystem::path& src) {
    ThrowIfSourceFileNotFound(src);
    if(FileUtils::IsSafeReadPath(src)) {
        if(tinyxml2::XMLDocument doc{}; auto* xml_root = ThrowIfSourceFileNotLoaded(doc, src)) {
            _adventure = std::move(std::make_unique<Adventure>(*xml_root));
        }
    }
}

void Game::LoadMaps() {
    LoadAdventureFromFile(default_adventure_src);
}

void Game::LoadEntities() {
    LoadEntitiesFromFile(default_entities_definition_src);
}

void Game::LoadItems() {
    LoadItemsFromFile(default_item_definition_src);
}

void Game::LoadDefaultTileDefinitions() {
    LoadTileDefinitionsFromFile(default_tile_definition_src);
}

void Game::LoadCursorsFromFile(const std::filesystem::path& src) {
    LoadCursorDefinitionsFromFile(src);
    _cursors.clear();
    for(const auto& c : CursorDefinition::GetLoadedDefinitions()) {
        _cursors.emplace_back(*c);
    }
}

void Game::LoadCursorDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    ThrowIfSourceFileNotFound(src);
    if(tinyxml2::XMLDocument doc{}; auto* xml_root = ThrowIfSourceFileNotLoaded(doc, src)) {
        DataUtils::ValidateXmlElement(*xml_root, "UI", "spritesheet", "", "cursors,overlays");
        _cursor_sheet.reset();
        CursorDefinition::DestroyCursorDefinitions();
        auto* xml_spritesheet = xml_root->FirstChildElement("spritesheet");
        _cursor_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet);
        if(auto* xml_cursors = xml_root->FirstChildElement("cursors")) {
            DataUtils::ForEachChildElement(*xml_cursors, "cursor",
                [this](const XMLElement& elem) {
                    CursorDefinition::CreateCursorDefinition(elem, _cursor_sheet);
                });
        }
    }
}

Material* Game::GetDefaultTileMaterial() const noexcept {
    return g_theRenderer->GetMaterial("Tile");
}

void Game::ThrowIfSourceFileNotFound(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    const auto error_msg = src.string() + " could not be found.";
    GUARANTEE_OR_DIE(FS::exists(src), error_msg.c_str());
}

void Game::LoadEntitiesFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    ThrowIfSourceFileNotFound(src);
    if(tinyxml2::XMLDocument doc{}; auto* xml_entities_root = ThrowIfSourceFileNotLoaded(doc, src)) {
        DataUtils::ValidateXmlElement(*xml_entities_root, "entities", "definitions,entity", "");
        if(auto* xml_definitions = xml_entities_root->FirstChildElement("definitions")) {
            DataUtils::ValidateXmlElement(*xml_definitions, "definitions", "", "src");
            const auto definitions_src = DataUtils::ParseXmlAttribute(*xml_definitions, "src", std::string{});
            GUARANTEE_OR_DIE(!definitions_src.empty(), "Entity definitions source is empty.");
            FS::path def_src(definitions_src);
            GUARANTEE_OR_DIE(FS::exists(def_src), "Entity definitions source not found.");
            LoadEntityDefinitionsFromFile(def_src);
        }
    }
}

XMLElement* Game::ThrowIfSourceFileNotLoaded(tinyxml2::XMLDocument& doc, const std::filesystem::path& src) {
    auto xml_result = doc.LoadFile(src.string().c_str());
    const auto error_msg = std::format("Entities source file at \"{}\" could not be loaded.", src.string());
    GUARANTEE_OR_DIE(xml_result == tinyxml2::XML_SUCCESS, error_msg.c_str());
    return doc.RootElement();
}

void Game::LoadEntityDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    ThrowIfSourceFileNotFound(src);
    if(tinyxml2::XMLDocument doc{}; auto* xml_root = ThrowIfSourceFileNotLoaded(doc, src)) {
        DataUtils::ValidateXmlElement(*xml_root, "entityDefinitions", "spritesheet,entityDefinition", "");
        _entity_sheet.reset();
        auto* xml_spritesheet = xml_root->FirstChildElement("spritesheet");
        _entity_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet);
        EntityDefinition::ClearEntityRegistry();
        DataUtils::ForEachChildElement(*xml_root, "entityDefinition",
            [this](const XMLElement& elem) {
                EntityDefinition::CreateEntityDefinition(elem, _entity_sheet);
            });
    }
}

void Game::LoadItemsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    ThrowIfSourceFileNotFound(src);
    tinyxml2::XMLDocument doc{};
    if(auto* xml_root = ThrowIfSourceFileNotLoaded(doc, src)) {
        DataUtils::ValidateXmlElement(*xml_root, "items", "spritesheet,item", "");
        _item_sheet.reset();
        Item::ClearItemRegistry();
        auto* xml_item_sheet = xml_root->FirstChildElement("spritesheet");
        _item_sheet = g_theRenderer->CreateSpriteSheet(*xml_item_sheet);
        DataUtils::ForEachChildElement(*xml_root, "item", [this](const XMLElement& elem) {
            ItemBuilder builder(elem, _item_sheet);
            builder.Build();
            });
    }
}

void Game::LoadTileDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    ThrowIfSourceFileNotFound(src);
    if(tinyxml2::XMLDocument doc{}; auto * xml_root = ThrowIfSourceFileNotLoaded(doc, src)) {
        DataUtils::ValidateXmlElement(*xml_root, "tileDefinitions", "spritesheet,tileDefinition", "");
        if(auto* xml_spritesheet = xml_root->FirstChildElement("spritesheet")) {
            if(_tileset_sheet) {
                return;
            }
            if(_tileset_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet); _tileset_sheet) {
                DataUtils::ForEachChildElement(*xml_root, "tileDefinition",
                    [&](const XMLElement& elem) {
                    if(auto* def = TileDefinition::CreateOrGetTileDefinition(elem, _tileset_sheet); def && def->GetSprite() && !def->GetSprite()->GetMaterial()) {
                        def->GetSprite()->SetMaterial(GetDefaultTileMaterial());
                    }
                });
            }
        }
    }
}

void Game::BeginFrame() noexcept {
    if(_nextGameState != _currentGameState) {
        OnExitState(_currentGameState);
        _currentGameState = _nextGameState;
        OnEnterState(_currentGameState);
    }
    switch(_currentGameState) {
    case GameState::Title:       BeginFrame_Title(); break;
    case GameState::Loading:     BeginFrame_Loading(); break;
    case GameState::Main:        BeginFrame_Main(); break;
    case GameState::Editor:      BeginFrame_Editor(); break;
    case GameState::Editor_Main: BeginFrame_EditorMain(); break;
    default:                 ERROR_AND_DIE("BEGIN FRAME UNDEFINED GAME STATE"); break;
    }
}

void Game::Update(TimeUtils::FPSeconds deltaSeconds) noexcept {
    switch(_currentGameState) {
    case GameState::Title:   Update_Title(deltaSeconds); break;
    case GameState::Loading: Update_Loading(deltaSeconds); break;
    case GameState::Main:    Update_Main(deltaSeconds); break;
    case GameState::Editor:  Update_Editor(deltaSeconds); break;
    case GameState::Editor_Main:  Update_EditorMain(deltaSeconds); break;
    default:                 ERROR_AND_DIE("UPDATE UNDEFINED GAME STATE"); break;
    }
}

bool Game::DoFade(const Rgba& color, TimeUtils::FPSeconds fadeTime, FullscreenEffect fadeType) {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(fadeType)) {
        _fullscreen_data.effectIndex = static_cast<int>(fadeType);
        curFadeTime = curFadeTime.zero();
    }

    _fullscreen_data.fadePercent = curFadeTime / fadeTime;
    _fullscreen_data.fadePercent = std::clamp(_fullscreen_data.fadePercent, 0.0f, 1.0f);
    const auto [r, g, b, a] = color.GetAsFloats();
    _fullscreen_data.fadeColor = Vector4{r, g, b, a};
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);

    curFadeTime += g_theRenderer->GetGameFrameTime();
    const auto isDone = _fullscreen_data.fadePercent == 1.0f;
    if(isDone && _fullscreen_callback) {
        _fullscreen_callback();
    }
    return isDone;
}

bool Game::DoFadeIn(const Rgba& color, TimeUtils::FPSeconds fadeTime) {
    return DoFade(color, fadeTime, FullscreenEffect::FadeIn);
}

bool Game::DoFadeOut(const Rgba& color, TimeUtils::FPSeconds fadeTime) {
    return DoFade(color, fadeTime, FullscreenEffect::FadeOut);
}

void Game::DoLumosity(float brightnessPower /*= 2.4f*/) {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::Lumosity)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Lumosity);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Lumosity);
    _fullscreen_data.lumosityBrightness = brightnessPower;
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::DoCircularGradient(float radius, const Rgba& color) {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::CircularGradient)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::CircularGradient);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::CircularGradient);
    _fullscreen_data.gradiantRadius = radius;
    const auto [r, g, b, a] = color.GetAsFloats();
    _fullscreen_data.gradiantColor = Vector4{r, g, b, a};
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::DoSepia() {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::Sepia)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Sepia);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Sepia);
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::DoSquareBlur() {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::SquareBlur)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::SquareBlur);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::SquareBlur);
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::StopFullscreenEffect() {
    static TimeUtils::FPSeconds curFadeTime{};
    _fullscreen_data.effectIndex = -1;
    _fullscreen_data.fadePercent = 0.0f;
    _fullscreen_data.fadeColor = Vector4::W_Axis;
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::UpdateFullscreenEffect(const FullscreenEffect& effect) {
    switch(effect) {
    case FullscreenEffect::None:
        StopFullscreenEffect();
        break;
    case FullscreenEffect::FadeIn:
        DoFadeIn(_fadeIn_color, _fadeInTime);
        break;
    case FullscreenEffect::FadeOut:
        DoFadeOut(_fadeOut_color, _fadeOutTime);
        break;
    case FullscreenEffect::Lumosity:
        DoLumosity(_fullscreen_data.lumosityBrightness);
        break;
    case FullscreenEffect::Sepia:
        DoSepia();
        break;
    case FullscreenEffect::CircularGradient:
        DoCircularGradient(_debug_gradientRadius, _debug_gradientColor);
        break;
    case FullscreenEffect::SquareBlur:
        DoSquareBlur();
        break;
    default:
        break;
    }
}

void Game::Render() const noexcept {
    switch(_currentGameState) {
    case GameState::Title:       Render_Title(); break;
    case GameState::Loading:     Render_Loading(); break;
    case GameState::Main:        Render_Main(); break;
    case GameState::Editor:      Render_Editor(); break;
    case GameState::Editor_Main: Render_EditorMain(); break;
    default:                 ERROR_AND_DIE("RENDER UNDEFINED GAME STATE"); break;
    }
}

void Game::EndFrame() noexcept {
    g_theRenderer->SetVSync(GetGameAs<Game>()->GetSettings().IsVsyncEnabled());
    switch(_currentGameState) {
    case GameState::Title:       EndFrame_Title(); break;
    case GameState::Loading:     EndFrame_Loading(); break;
    case GameState::Main:        EndFrame_Main(); break;
    case GameState::Editor:      EndFrame_Editor(); break;
    case GameState::Editor_Main: EndFrame_EditorMain(); break;
    default:                 ERROR_AND_DIE("END FRAME UNDEFINED GAME STATE"); break;
    }
}

bool Game::HasCursor(const std::string& name) const noexcept {
    const auto found_cursor = std::find_if(std::begin(_cursors), std::end(_cursors), [name](const Cursor& c) { return c.GetDefinition()->name == name; });
    return found_cursor != std::end(_cursors);
}

bool Game::HasCursor(CursorId id) const noexcept {
    const auto idAsIndex = static_cast<std::size_t>(id);
    return idAsIndex < _cursors.size();
}

void Game::SetCurrentCursorByName(const std::string& name) noexcept {
    const auto found_cursor = std::find_if(std::begin(_cursors), std::end(_cursors), [name](const Cursor& c) { return c.GetDefinition()->name == name; });
    if(found_cursor != std::end(_cursors)) {
        current_cursor = &(*found_cursor);
    }
}

void Game::SetCurrentCursorById(CursorId id) noexcept {
    GUARANTEE_OR_DIE(!_cursors.empty(), "Cursors array is empty!!");
    current_cursor = &_cursors[static_cast<std::size_t>(id)];
}

bool Game::IsDebugging() const noexcept {
#ifdef DEBUG_BUILD
    return _debug_render;
#else
    return false;
#endif
}

void Game::HandlePlayerInput() {
    HandlePlayerKeyboardInput();
    HandlePlayerControllerInput();
    HandlePlayerMouseInput();
}

void Game::HandlePlayerKeyboardInput() {
    const bool is_right = g_theInputSystem->WasKeyJustPressed(KeyCode::D) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::Right) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad6);
    const bool is_right_held = g_theInputSystem->IsKeyDown(KeyCode::D) ||
        g_theInputSystem->IsKeyDown(KeyCode::Right) ||
        g_theInputSystem->IsKeyDown(KeyCode::NumPad6);

    const bool is_left = g_theInputSystem->WasKeyJustPressed(KeyCode::A) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::Left) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad4);
    const bool is_left_held = g_theInputSystem->IsKeyDown(KeyCode::A) ||
        g_theInputSystem->IsKeyDown(KeyCode::Left) ||
        g_theInputSystem->IsKeyDown(KeyCode::NumPad4);

    const bool is_up = g_theInputSystem->WasKeyJustPressed(KeyCode::W) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::Up) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad8);
    const bool is_up_held = g_theInputSystem->IsKeyDown(KeyCode::W) ||
        g_theInputSystem->IsKeyDown(KeyCode::Up) ||
        g_theInputSystem->IsKeyDown(KeyCode::NumPad8);

    const bool is_down = g_theInputSystem->WasKeyJustPressed(KeyCode::S) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::Down) ||
        g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad2);
    const bool is_down_held = g_theInputSystem->IsKeyDown(KeyCode::S) ||
        g_theInputSystem->IsKeyDown(KeyCode::Down) ||
        g_theInputSystem->IsKeyDown(KeyCode::NumPad2);

    const bool is_upright = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad9) || (is_right && is_up);
    const bool is_upleft = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad7) || (is_left && is_up);
    const bool is_downright = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad3) || (is_right && is_down);
    const bool is_downleft = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad1) || (is_left && is_down);

    const bool is_shift = g_theInputSystem->IsKeyDown(KeyCode::Shift);
    const bool is_rest = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad5)
        || g_theInputSystem->WasKeyJustPressed(KeyCode::Z);

    if(is_shift) {
        if(is_right_held) {
            _adventure->CurrentMap()->cameraController.Translate(Vector2::X_Axis);
        } else if(is_left_held) {
            _adventure->CurrentMap()->cameraController.Translate(-Vector2::X_Axis);
        }

        if(is_up_held) {
            _adventure->CurrentMap()->cameraController.Translate(-Vector2::Y_Axis);
        } else if(is_down_held) {
            _adventure->CurrentMap()->cameraController.Translate(Vector2::Y_Axis);
        }
        return;
    }

    auto player = _adventure->CurrentMap()->player;
    if(is_rest) {
        player->Act();
        return;
    }
    if(is_upright) {
        _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetNorthEastNeighbor());
    } else if(is_upleft) {
        _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetNorthWestNeighbor());
    } else if(is_downright) {
        _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetSouthEastNeighbor());
    } else if(is_downleft) {
        _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetSouthWestNeighbor());
    } else {
        if(is_right) {
            _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetEastNeighbor());
        } else if(is_left) {
            _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetWestNeighbor());
        }

        if(is_up) {
            _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetNorthNeighbor());
        } else if(is_down) {
            _adventure->CurrentMap()->MoveOrAttack(player, player->tile->GetSouthNeighbor());
        }
    }
}

void Game::HandlePlayerMouseInput() {
    if(g_theUISystem->WantsInputMouseCapture()) {
        return;
    }
    static bool requested_zoom_out = false;
    static bool requested_zoom_in = false;
    if(g_theInputSystem->WasMouseWheelJustScrolledUp()) {
        requested_zoom_in = true;
    }
    if(g_theInputSystem->WasMouseWheelJustScrolledDown()) {
        requested_zoom_out = true;
    }
    if(requested_zoom_out) {
        SetFullscreenEffect(FullscreenEffect::FadeOut, [this]() {
            ZoomOut();
            requested_zoom_out = false;
            SetFullscreenEffect(FullscreenEffect::FadeIn, []() {});
            });
    }
    if(requested_zoom_in) {
        SetFullscreenEffect(FullscreenEffect::FadeOut, [this]() {
            ZoomIn();
            requested_zoom_in = false;
            SetFullscreenEffect(FullscreenEffect::FadeIn, []() {});
            });
    }
}

void Game::HandlePlayerControllerInput() {
    auto& controller = g_theInputSystem->GetXboxController(0);
    Vector2 rthumb = controller.GetRightThumbPosition();
    rthumb.y *= static_cast<float>(GetGameAs<Game>()->GetSettings().IsMouseInvertedY()) ? 1.0f : -1.0f;
    _adventure->CurrentMap()->cameraController.Translate(gameOptions.GetCameraSpeed() * rthumb * g_theRenderer->GetGameFrameTime().count());

    if(controller.WasButtonJustPressed(XboxController::Button::RightThumb)) {
        _adventure->CurrentMap()->FocusEntity(_adventure->CurrentMap()->player);
    }

    auto ltrigger = controller.GetLeftTriggerPosition();
    auto rtrigger = controller.GetRightTriggerPosition();
    if(ltrigger > 0.0f) {
        ZoomOut();
    }
    if(rtrigger > 0.0f) {
        ZoomIn();
    }
}

void Game::ZoomOut() {
    _adventure->CurrentMap()->ZoomOut();
}

void Game::ZoomIn() {
    _adventure->CurrentMap()->ZoomIn();
}

void Game::HandleDebugInput() {
#ifdef PROFILE_BUILD
    if(_debug_show_debug_window) {
        ShowDebugUI();
    }
    HandleDebugKeyboardInput();
    HandleDebugMouseInput();
#endif
}

void Game::HandleDebugKeyboardInput() {
#ifdef PROFILE_BUILD
    if(g_theUISystem->WantsInputKeyboardCapture()) {
        return;
    }
    if(!_debug_show_debug_window && !g_theUISystem->IsAnyImguiDebugWindowVisible()) {
        g_theInputSystem->HideMouseCursor();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::J)) {
        if(!g_theInputSystem->IsMouseLockedToViewport()) {
            g_theInputSystem->LockMouseToViewport(*g_theRenderer->GetOutput()->GetWindow());
        } else {
            g_theInputSystem->UnlockMouseFromViewport();
        }
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        _debug_show_debug_window = !_debug_show_debug_window;
        if(!g_theInputSystem->IsMouseCursorVisible()) {
            g_theInputSystem->ShowMouseCursor();
        }
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F2)) {
        /* DO NOTHING */
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F4)) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F5)) {
        static bool is_fullscreen = false;
        is_fullscreen = !is_fullscreen;
        g_theRenderer->SetFullscreen(is_fullscreen);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F6)) {
        g_theUISystem->ToggleImguiMetricsWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F9)) {
        g_theRenderer->RequestScreenShot();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::P)) {
        _adventure->CurrentMap()->SetPriorityLayer(MathUtils::GetRandomLessThan(_adventure->CurrentMap()->GetLayerCount()));
    }

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::G)) {
        _adventure->CurrentMap()->RegenerateMap();
    }

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::B)) {
        _adventure->CurrentMap()->cameraController.DoCameraShake([]()->float { const auto t = g_theRenderer->GetGameTime().count(); return std::cos(t) * std::sin(t); });
    }
#endif
}

void Game::HandleDebugMouseInput() {
#ifdef PROFILE_BUILD
    if(g_theUISystem->WantsInputMouseCapture()) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::LButton)) {
        const auto& picked_tiles = DebugGetTilesFromCursor();
        _debug_has_picked_tile_with_click = _debug_show_tile_debugger && picked_tiles.has_value();
        _debug_has_picked_entity_with_click = _debug_show_entity_debugger && picked_tiles.has_value();
        _debug_has_picked_feature_with_click = _debug_show_feature_debugger && picked_tiles.has_value();
        if(_debug_has_picked_tile_with_click) {
            _debug_inspected_tiles = (*picked_tiles);
        }
        if(_debug_has_picked_entity_with_click) {
            _debug_inspected_entity = (*picked_tiles)[0]->actor;
            _debug_has_picked_entity_with_click = _debug_inspected_entity != nullptr;
        }
        if(_debug_has_picked_feature_with_click) {
            _debug_inspected_feature = (*picked_tiles)[0]->feature;
            _debug_has_picked_feature_with_click = _debug_inspected_feature != nullptr;
        }
    }
    if(g_theInputSystem->IsKeyDown(KeyCode::RButton)) {
        if(const auto* tile = _adventure->CurrentMap()->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile != nullptr) {
            _adventure->CurrentMap()->player->SetPosition(tile->GetCoords());
        }
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::MButton)) {
        if(auto* tile = _adventure->CurrentMap()->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile != nullptr) {
            tile->SetEntity(Feature::GetFeatureByName("flamePedestal"));
        }
    }
#endif
}

#pragma region Imgui UI Debugger Code

#ifdef PROFILE_BUILD

void Game::ShowDebugUI() {
    ImGui::SetNextWindowSize(Vector2{550.0f, 500.0f}, ImGuiCond_Always);
    if(ImGui::Begin("Debugger", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        if(ImGui::BeginTabBar("Debugger", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_FittingPolicyScroll)) {
            ShowFrameInspectorUI();
            ShowWorldInspectorUI();
            ShowEffectsDebuggerUI();
            ShowTileDebuggerUI();
            ShowEntityDebuggerUI();
            ShowFeatureDebuggerUI();
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void Game::ShowTileDebuggerUI() {
    if(_debug_show_tile_debugger = ImGui::BeginTabItem("Tile", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder); _debug_show_tile_debugger) {
        ShowTileInspectorUI();
        ImGui::EndTabItem();
    }
}

void Game::ShowEffectsDebuggerUI() {
    if(ImGui::BeginTabItem("Effects", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder)) {
        ShowEffectsUI();
        ImGui::EndTabItem();
    }
}

void Game::ShowEffectsUI() {
    static std::string current_item = "None";
    if(ImGui::BeginCombo("Shader Effect", current_item.c_str())) {
        bool is_selected = false;
        if(ImGui::Selectable("None")) {
            is_selected = true;
            current_item = "None";
            _current_fs_effect = FullscreenEffect::None;
        }
        if(ImGui::Selectable("Fade In")) {
            is_selected = true;
            current_item = "Fade In";
            _current_fs_effect = FullscreenEffect::FadeIn;
        }
        if(ImGui::Selectable("Fade Out")) {
            is_selected = true;
            current_item = "Fade Out";
            _current_fs_effect = FullscreenEffect::FadeOut;
        }
        if(ImGui::Selectable("Lumosity")) {
            is_selected = true;
            current_item = "Lumosity";
            _current_fs_effect = FullscreenEffect::Lumosity;
        }
        if(ImGui::Selectable("Sepia")) {
            is_selected = true;
            current_item = "Sepia";
            _current_fs_effect = FullscreenEffect::Sepia;
        }
        if(ImGui::Selectable("CircularGradiant")) {
            is_selected = true;
            current_item = "CircularGradiant";
            _current_fs_effect = FullscreenEffect::CircularGradient;
        }
        if(ImGui::Selectable("SquareBlur")) {
            is_selected = true;
            current_item = "SquareBlur";
            _current_fs_effect = FullscreenEffect::SquareBlur;
        }
        if(is_selected) {
            ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    switch(_current_fs_effect) {
    case FullscreenEffect::None:
        ImGui::Text("Effect: None");
        break;
    case FullscreenEffect::FadeIn:
        ImGui::Text("Effect: Fade In");
        (void)ImGui::ColorEdit4("Fade In Color##Picker", _fadeIn_color, ImGuiColorEditFlags_NoLabel); //I don't care if the value changed.
        if(ImGui::InputFloat("Fade In Time (s)", &_debug_fadeInTime)) {
            _fadeInTime = TimeUtils::FPSeconds{_debug_fadeInTime};
        }
        break;
    case FullscreenEffect::FadeOut:
        ImGui::Text("Effect: Fade Out");
        (void)ImGui::ColorEdit4("Fade Out Color##Picker", _fadeOut_color, ImGuiColorEditFlags_NoLabel); //I don't care if the value changed.
        if(ImGui::InputFloat("Fade Out Time (s)", &_debug_fadeOutTime)) {
            _fadeOutTime = TimeUtils::FPSeconds{_debug_fadeOutTime};
        }
        break;
    case FullscreenEffect::Lumosity:
        ImGui::Text("Effect: Lumosity");
        ImGui::DragFloat("Brightness##Lumosity", &_fullscreen_data.lumosityBrightness, 0.25f, 0.0f, 15.0f);
        break;
    case FullscreenEffect::Sepia:
        ImGui::Text("Effect: Sepia");
        break;
    case FullscreenEffect::CircularGradient:
        ImGui::Text("Effect: CircularGradient");
        (void)ImGui::ColorEdit4("Gradient Color##Picker", _debug_gradientColor, ImGuiColorEditFlags_NoLabel); //I don't care if the value changed.
        if(ImGui::DragFloat("Radius##Circle", &_fullscreen_data.gradiantRadius, 1.0f, 0.0f, 1.0f)) {
            _fullscreen_data.gradiantRadius = _debug_gradientRadius;
        }
        break;
    case FullscreenEffect::SquareBlur:
        ImGui::Text("Effect: SquareBlur");
        break;
    default:
        ImGui::Text("Effect Stopped");
        break;
    }
}

void Game::ShowEntityDebuggerUI() {
    if(_debug_show_entity_debugger = ImGui::BeginTabItem("Entity", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder); _debug_show_entity_debugger) {
        ShowEntityInspectorUI();
        ImGui::EndTabItem();
    }
}

void Game::ShowFeatureDebuggerUI() {
    if(_debug_show_feature_debugger = ImGui::BeginTabItem("Feature", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder); _debug_show_feature_debugger) {
        ShowFeatureInspectorUI();
        ImGui::EndTabItem();
    }
}

void Game::ShowFrameInspectorUI() {
    static constexpr std::size_t max_histogram_count = 60;
    static std::array<float, max_histogram_count> histogram{};
    static std::size_t histogramIndex = 0;
    static const std::string histogramLabel = std::format("Last {} Frames", max_histogram_count);
    if(ImGui::BeginTabItem("Frame Data", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder)) {
        const auto frameTime = g_theRenderer->GetGameFrameTime().count();
        histogram[histogramIndex++] = frameTime;
        ImGui::Text("FPS: %0.1f", 1.0f / frameTime);
        ImGui::PlotHistogram(histogramLabel.c_str(), histogram.data(), static_cast<int>(histogram.size()));
        ImGui::Text("Frame time: %0.7f", frameTime);
        ImGui::Text("Min: %0.7f", *std::min_element(std::begin(histogram), std::end(histogram)));
        ImGui::Text("Max: %0.7f", *std::max_element(std::begin(histogram), std::end(histogram)));
        ImGui::Text("Avg: %0.7f", std::reduce(std::begin(histogram), std::end(histogram), 0.0f) / max_histogram_count);
        static bool is_vsync_enabled = GetGameAs<Game>()->GetSettings().IsVsyncEnabled();
        if(ImGui::Checkbox("Vsync", &is_vsync_enabled)) {
            GetGameAs<Game>()->GetSettings().SetVsyncEnabled(is_vsync_enabled);
        }
        if(ImGui::Button("Take Screenshot")) {
            RequestScreenShot();
        }
        ImGui::EndTabItem();
    }
    histogramIndex %= max_histogram_count;
}

void Game::ShowWorldInspectorUI() {
    if(ImGui::BeginTabItem("World", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder)) {
        ImGui::Text("View height: %.0f", _adventure->CurrentMap()->cameraController.GetCamera().GetViewHeight());
        ImGui::Text("World dimensions: %.0f, %.0f", _adventure->CurrentMap()->CalcMaxDimensions().x, _adventure->CurrentMap()->CalcMaxDimensions().y);
        ImGui::Text("Camera: [%.1f,%.1f]", _adventure->CurrentMap()->cameraController.GetCamera().position.x, _adventure->CurrentMap()->cameraController.GetCamera().position.y);
        {
            const auto& mouse_coords = g_theRenderer->ConvertScreenToWorldCoords(_adventure->CurrentMap()->cameraController.GetCamera(), g_theInputSystem->GetMouseCoords());
            ImGui::Text("Mouse: [%.1f,%.1f]", mouse_coords.x, mouse_coords.y);
            const auto& cursor_coords = current_cursor->GetCoords();
            ImGui::Text("Cursor: [%d,%d]", cursor_coords.x, cursor_coords.y);
        }
        ImGui::Text("Tiles in view: %llu", _adventure->CurrentMap()->DebugTilesInViewCount());
        ImGui::Text("Tiles visible in view: %llu", _adventure->CurrentMap()->DebugVisibleTilesInViewCount());
        static bool show_camera = false;
        ImGui::Checkbox("Show Camera", &show_camera);
        _debug_show_camera = show_camera;
        static bool show_grid = false;
        ImGui::Checkbox("World Grid", &show_grid);
        _debug_show_grid = show_grid;
        ImGui::SameLine();
        if(ImGui::ColorEdit4("Grid Color##Picker", _grid_color, ImGuiColorEditFlags_NoLabel)) {
            _adventure->CurrentMap()->SetDebugGridColor(_grid_color);
        }
        static bool show_world_bounds = false;
        ImGui::Checkbox("World Bounds", &show_world_bounds);
        _debug_show_world_bounds = show_world_bounds;
        static bool show_camera_bounds = false;
        ImGui::Checkbox("Camera Bounds", &show_camera_bounds);
        _debug_show_camera_bounds = show_camera_bounds;
        static bool show_room_bounds = false;
        ImGui::Checkbox("Show Room Bounds", &show_room_bounds);
        _debug_show_room_bounds = show_room_bounds;
        static bool show_all_entities = false;
        ImGui::Checkbox("Show All Entities", &show_all_entities);
        _debug_show_all_entities = show_all_entities;
        static bool show_raycasts = false;
        ImGui::Checkbox("Show raycasts", &show_raycasts);
        _debug_show_raycasts = show_raycasts;
        _debug_render = _debug_show_room_bounds || _debug_show_camera || _debug_show_grid || _debug_show_world_bounds || _debug_show_camera_bounds || _debug_show_all_entities || _debug_show_raycasts;
        static int light_level = this->_adventure->CurrentMap()->GetCurrentGlobalLightValue();
        if (ImGui::SliderInt("Global Light", &light_level, min_light_value, max_light_value, "%d", ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_NoRoundToFormat)) {
            if (_adventure->CurrentMap()->AllowLightingDuringDay()) {
                auto& m = *(this->_adventure->CurrentMap());
                m.SetDebugGlobalLight(light_level);
                m.SetSkyColorFromGlobalLight();
            }
        }
        static bool always_daytime = true;
        if (ImGui::Checkbox("Disable lighting", &always_daytime)) {
            auto& m = *(this->_adventure->CurrentMap());
            m.DebugDisableLighting(!always_daytime);
            
        }
        ImGui::EndTabItem();
    }
}

void Game::ShowTileInspectorUI() {
    if(const auto& picked_tiles = DebugGetTilesFromCursor(); !picked_tiles.has_value()) {
        ImGui::Text("Tile Inspector: Invalid Tiles");
        return;
    } else {
        bool has_tile = !picked_tiles->empty();
        bool has_selected_tile = _debug_has_picked_tile_with_click && !_debug_inspected_tiles.empty();
        bool shouldnt_show_inspector = !(has_tile || has_selected_tile);
        if(shouldnt_show_inspector) {
            ImGui::Text("Tile Inspector: None");
            return;
        }
        ImGui::Text("Tile Inspector");
        ImGui::SameLine();
        const auto& tiles = has_selected_tile ? _debug_inspected_tiles : (*picked_tiles);
        ImGui::PushID(static_cast<int>(tiles[0]->GetIndexFromCoords()));
        if(ImGui::Button("Unlock")) {
            _debug_has_picked_tile_with_click = false;
            _debug_inspected_tiles.clear();
        }
        ImGui::PopID();
        const auto tiles_per_row = std::size_t{3u};
        const auto tiles_per_col = std::size_t{3u};
        //TODO: Get Centering working. Not currently in API.
        //const auto debugger_window_content_region_width = ImGui::GetWindowContentRegionWidth();
        //const auto debugger_window_width = ImGui::GetWindowWidth();
        //ImGui::SetNextItemWidth(debugger_window_content_region_width * 0.5f);
        ShowTileInspectorTableUI(tiles, tiles_per_row, tiles_per_col);
    }
}

void Game::ShowTileInspectorStatsTableUI(const TileDefinition* cur_def, const Tile* cur_tile) {
    if(cur_tile == nullptr || cur_def == nullptr) {
        return;
    }
    ImGui::PushID(cur_tile);
    if(ImGui::BeginTable("TileInspectorTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("TileInspectorTableRowIdsColumn");
        ImGui::TableSetupColumn("TileInspectorTableRowValuesColumn");

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Layer Index:");
        ImGui::TableNextColumn();
        ImGui::Text(std::to_string(cur_tile->layer->z_index).c_str());

        {
            const auto types = StringUtils::Split(cur_def->name, '.', false);
            const auto type = [&]()->std::string { if(types.size() > 0) return types[0]; else return std::string{}; }();
            const auto subtype = [&]()->std::string { if(types.size() > 1) return types[1]; else return std::string{}; }();

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Type:");
            ImGui::TableNextColumn();
            ImGui::Text(type.c_str());

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("SubType:");
            ImGui::TableNextColumn();
            ImGui::Text(subtype.c_str());
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Light Value:");
        ImGui::TableNextColumn();
        ImGui::Text(std::to_string(cur_tile->GetLightValue()).c_str());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Can See:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_tile && cur_tile->CanSee() ? "true" : "false"));

        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Is At Edge:");
            ImGui::TableNextColumn();
            TileInfo ti{cur_tile->layer, cur_tile->GetIndexFromCoords()};
            ImGui::Text((ti.IsAtEdge() ? "true" : "false"));
        }
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Is Sky:");
            ImGui::TableNextColumn();
            TileInfo ti{cur_tile->layer, cur_tile->GetIndexFromCoords()};
            ImGui::Text((ti.IsSky() ? "true" : "false"));
        }

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Visible:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_def->is_visible ? "true" : "false"));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Opaque:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_def->is_opaque ? "true" : "false"));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Solid:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_def->is_solid ? "true" : "false"));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Transparent:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_def->is_transparent ? "true" : "false"));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Animated:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_def->is_animated ? "true" : "false"));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Entrance:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_def->is_entrance ? "true" : "false"));

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Exit:");
        ImGui::TableNextColumn();
        ImGui::Text((cur_def->is_exit ? "true" : "false"));

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void Game::ShowTileInspectorTableUI(const std::vector<Tile*>& tiles, const uint8_t tiles_per_row, const uint8_t tiles_per_col) {
    if(ImGui::BeginTable("TileInspectorTable", tiles_per_col, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX)) {
        ImGui::TableSetupColumn("TileInspectorTableLeftColumn", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("TileInspectorTableCenterColumn", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("TileInspectorTableRightColumn", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        for(auto row = std::size_t{0u}; row != tiles_per_row; ++row) {
            ImGui::TableNextRow(ImGuiTableRowFlags_None, 100.0f);
            for(auto col = std::size_t{0u}; col != tiles_per_col; ++col) {
                ImGui::TableSetColumnIndex(static_cast<int>(col));
                const auto index = row * tiles_per_col + col;
                if(index >= tiles.size()) {
                    continue;
                }
                if(const auto* cur_tile = tiles[index]; cur_tile == nullptr) {
                    continue;
                } else {
                    if(const auto* cur_def = TileDefinition::GetTileDefinitionByName(cur_tile->GetType()); cur_def == nullptr) {
                        cur_def = TileDefinition::GetTileDefinitionByName("void");
                    } else {
                        if(const auto* cur_sprite = cur_def->GetSprite()) {
                            ShowInspectedElementImageUI(cur_sprite, Vector2::One * 100.0f, cur_sprite->GetCurrentTexCoords());
                            if(ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ShowTileInspectorStatsTableUI(cur_def, cur_tile);
                                ImGui::EndTooltip();
                            }
                        }
                    }
                }
            }
        }
        ImGui::EndTable();
    }
}

void Game::ShowInspectedElementImageUI(const AnimatedSprite* cur_sprite, const Vector2& dims, const AABB2& tex_coords) noexcept {
    ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
}

std::optional<std::vector<Tile*>> Game::DebugGetTilesFromCursor() {
    if(!current_cursor) {
        return {};
    }
    if(_debug_has_picked_tile_with_click) {
        static std::optional<std::vector<Tile*>> picked_tiles{};
        if(picked_tiles = _adventure->CurrentMap()->PickTilesFromWorldCoords(Vector2{current_cursor->GetCoords()}); !picked_tiles.has_value()) {
            return {};
        }
        auto* tile_actor = (*picked_tiles)[0]->actor;
        auto* tile_feature = (*picked_tiles)[0]->feature;
        bool tile_has_entity = tile_actor || tile_feature;
        if(tile_has_entity && _debug_has_picked_entity_with_click) {
            if(tile_actor) {
                _debug_inspected_entity = tile_actor;
            } else if(tile_feature) {
                _debug_inspected_entity = tile_feature;
            }
        }
        return picked_tiles;
    }
    return _adventure->CurrentMap()->PickTilesFromWorldCoords(Vector2{current_cursor->GetCoords()});
}

void Game::ShowEntityInspectorUI() {
    if(const auto& picked_tiles = DebugGetTilesFromCursor(); !picked_tiles.has_value()) {
        ImGui::Text("Entity Inspector: Invalid Tiles");
        return;
    } else {
        const auto picked_count = (*picked_tiles).size();
        bool has_entity = (picked_count > 0 && (*picked_tiles)[0]->actor);
        bool has_selected_entity = _debug_has_picked_entity_with_click && _debug_inspected_entity;
        bool shouldnt_show_inspector = !has_entity && !has_selected_entity;
        if(shouldnt_show_inspector) {
            ImGui::Text("Entity Inspector: None");
            return;
        }
        if(auto* const cur_entity = _debug_inspected_entity ? _debug_inspected_entity : (*picked_tiles)[0]->actor) {
            if(const auto* cur_sprite = cur_entity->sprite) {
                ImGui::Text("Entity Inspector");
                ImGui::SameLine();
                ImGui::PushID(cur_entity);
                if(ImGui::Button("Unlock")) {
                    _debug_has_picked_entity_with_click = false;
                    _debug_inspected_entity = nullptr;
                }
                ImGui::PopID();
                ImGui::SameLine();
                ShowEntityInspectorInventoryManipulatorUI(cur_entity);

                if(ImGui::BeginTable("EntityInspectorTable", 2, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("Entity");
                    ImGui::TableSetupColumn("Inventory");
                    //ImGui::TableHeadersRow(); //Commented out to hide header row.
                    ImGui::TableNextColumn();
                    ShowEntityInspectorEntityColumnUI(cur_entity, cur_sprite);
                    ImGui::TableNextColumn();
                    ShowEntityInspectorInventoryColumnUI(cur_entity);
                    ImGui::EndTable();
                }
            }
        }
    }
}

void Game::ShowFeatureInspectorUI() {
    if(const auto& picked_tiles = DebugGetTilesFromCursor(); !picked_tiles.has_value()) {
        ImGui::Text("Feature Inspector: Invalid Tiles");
        return;
    } else {
        const auto picked_count = picked_tiles->size();
        bool has_feature = (picked_count > 0 && (*picked_tiles)[0]->feature);
        bool has_selected_feature = _debug_has_picked_feature_with_click && _debug_inspected_feature;
        bool shouldnt_show_inspector = !has_feature && !has_selected_feature;
        if(shouldnt_show_inspector) {
            ImGui::Text("Feature Inspector: None");
            return;
        }
        if(const auto* cur_entity = _debug_inspected_feature ? _debug_inspected_feature : (*picked_tiles)[0]->feature) {
            if(const auto* cur_sprite = cur_entity->sprite) {
                ImGui::Text("Feature Inspector");
                ImGui::SameLine();
                ImGui::PushID(cur_entity);
                if(ImGui::Button("Unlock")) {
                    _debug_has_picked_feature_with_click = false;
                    _debug_inspected_feature = nullptr;
                }
                ImGui::PopID();
                ImGui::NewLine();
                ShowEntityInspectorImageUI(cur_sprite, cur_entity);
                ImGui::SameLine();
                ShowFeatureInspectorFeatureStatesUI(cur_entity);
            }
        }
    }
}

void Game::ShowFeatureInspectorFeatureStatesUI(const Entity* cur_entity) {
    if(const auto* feature = dynamic_cast<const Feature*>(cur_entity); feature != nullptr) {
        auto info = FeatureInfo{feature->layer, feature->parent_tile->GetIndexFromCoords()};
        if(info.HasStates()) {
            if(ImGui::BeginTable("FeatureInspectorTable", 2, ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("Active");
                ImGui::TableSetupColumn("States");
                ImGui::TableHeadersRow();
                for(const auto& state : info.GetStates()) {
                        ImGui::TableNextColumn();
                        if(const auto disable = info.GetCurrentState() == state; disable) {
                            ImGui::BeginDisabled(disable);
                            ImGui::Button("Set##FeatureInspectorTableDeactivatedButton");
                            ImGui::EndDisabled();
                        } else {
                            ImGui::BeginDisabled(disable);
                            if(ImGui::Button("Set##FeatureInspectorTableActivatedButton")) {
                                info.SetState(state);
                            }
                            ImGui::EndDisabled();
                        }
                        ImGui::TableNextColumn();
                        ImGui::Text(state.c_str());
                    }
                }
            ImGui::EndTable();
        }
    }
}

void Game::ShowEntityInspectorEntityColumnUI(const Entity* cur_entity, const AnimatedSprite* cur_sprite) {
    std::ostringstream ss;
    ss << "Name: " << cur_entity->name;
    if(auto* def = EntityDefinition::GetEntityDefinitionByName(cur_entity->name)) {
        ss << "\nInvisible: " << (def->is_invisible ? "true" : "false");
        ss << "\nAnimated: " << (def->is_animated ? "true" : "false");
    }
    ImGui::Text(ss.str().c_str());
    ImGui::NewLine();
    ShowEntityInspectorImageUI(cur_sprite, cur_entity);
}

void Game::ShowEntityInspectorInventoryManipulatorUI(Entity* const cur_entity) {
    static std::string current_item_fname = "";
    static Item* current_item_ptr = nullptr;
    if(ImGui::BeginCombo("Inventory", current_item_fname.c_str())) {
        bool is_selected{false};
        if(ImGui::Selectable("None")) {
            current_item_fname.clear();
            current_item_ptr = nullptr;
        }
        for(const auto& item : Item::s_registry) {
            ImGui::PushID(item.second.get());
            if(ImGui::Selectable(item.second.get()->GetFriendlyName().c_str())) {
                current_item_fname = item.second.get()->GetFriendlyName();
                current_item_ptr = item.second.get();
            }
            ImGui::PopID();
            if(is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if(ImGui::Button("Add Item")) {
        if(current_item_ptr) {
            cur_entity->inventory.AddItem(current_item_ptr->GetName());
        }
    }
}

void Game::ShowEntityInspectorInventoryColumnUI(Entity* const cur_entity) {
    if(!cur_entity) {
        return;
    }
    std::ostringstream ss;
    if(cur_entity->inventory.empty()) {
        ss << "Inventory: None";
        ImGui::Text(ss.str().c_str());
        return;
    }
    ImGui::PushID(cur_entity);
    if(ImGui::BeginTable("Inventory", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedSame )) {
        ImGui::TableSetupColumn("Inventory");
        ImGui::TableSetupColumn("Count");
        ImGui::TableHeadersRow();
        std::size_t item_number{0u};
        for(auto* item : cur_entity->inventory) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text(item->GetFriendlyName().c_str());
            ImGui::SameLine();
            const auto button_label = [&]() -> std::string {
                if(auto* const asActor = dynamic_cast<Actor* const>(cur_entity); asActor) {
                    if(item->GetEquipSlot() != EquipSlot::None) {
                        if(asActor->inventory.HasItem(item)) {
                            if(asActor->IsEquipped(item->GetEquipSlot(), item->GetName())) {
                                return {"Unequip"};
                            } else {
                                return {"Equip"};
                            }
                        }
                    }
                }
                return {};
            }(); //IIIL
            if(!button_label.empty()) {
                ImGui::PushID(item);
                if(ImGui::SmallButton(button_label.c_str())) {
                    if(auto* const asActor = dynamic_cast<Actor* const>(cur_entity); asActor) {
                        if(auto* equipped_item = asActor->IsEquipped(item->GetEquipSlot()); equipped_item != nullptr) {
                            if(!asActor->IsEquipped(item->GetEquipSlot(), item->GetName())) {
                                asActor->Unequip(equipped_item->GetEquipSlot());
                                asActor->Equip(item->GetEquipSlot(), item);
                            } else {
                                asActor->Unequip(equipped_item->GetEquipSlot());
                            }
                        } else {
                            asActor->Equip(item->GetEquipSlot(), item);
                        }
                    }
                }
                ImGui::PopID();
            }
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text(std::to_string(item->GetCount()).c_str());
            ImGui::SameLine();
            ImGui::PushButtonRepeat(true);
            ImGui::PushID(item);
            if(ImGui::ArrowButton("##Up", ImGuiDir_Up)) {
                item->IncrementCount();
            }
            ImGui::PopID();
            ImGui::PopButtonRepeat();
            ImGui::SameLine();
            ImGui::PushButtonRepeat(true);
            ImGui::PushID(item);
            if(ImGui::ArrowButton("##Down", ImGuiDir_Down)) {
                item->DecrementCount();
            }
            ImGui::PopID();
            ImGui::PopButtonRepeat();
            ++item_number;
        }

        ImGui::EndTable();
    }
    ImGui::PopID();
}

void Game::ShowEntityInspectorImageUI(const AnimatedSprite* cur_sprite, const Entity* cur_entity) {
    ShowInspectedActorEquipmentOnlyImageUI(cur_sprite, cur_entity, EquipSlot::Cape);
    ImGui::SameLine(8.0f);
    ShowInspectedElementImageUI(cur_sprite, Vector2::One * 100.0f, cur_sprite->GetCurrentTexCoords());
    ShowInspectedActorEquipmentExceptImageUI(cur_sprite, cur_entity, EquipSlot::Cape);
}

void Game::ShowInspectedActorEquipmentExceptImageUI(const AnimatedSprite* cur_sprite, const Entity* cur_entity, const EquipSlot& skip_equip_slot /*= EquipSlot::None*/) noexcept {
    if(const auto* actor = dynamic_cast<const Actor*>(cur_entity)) {
        for(const auto& eq : actor->GetEquipment()) {
            if(eq && eq->GetEquipSlot() != skip_equip_slot) {
                ImGui::SameLine(8.0f);
                ImGui::SetItemAllowOverlap();
                const auto eq_coords = eq->GetSprite()->GetCurrentTexCoords();
                ShowInspectedElementImageUI(cur_sprite, Vector2::One * 100.0f, eq_coords);
            }
        }
    }
}

void Game::ShowInspectedActorEquipmentOnlyImageUI(const AnimatedSprite* cur_sprite, const Entity* cur_entity, const EquipSlot& equip_slot) noexcept {
    if(const auto* actor = dynamic_cast<const Actor*>(cur_entity)) {
        for(const auto& eq : actor->GetEquipment()) {
            if(eq && eq->GetEquipSlot() == equip_slot) {
                ImGui::SameLine(8.0f);
                ImGui::SetItemAllowOverlap();
                const auto eq_coords = eq->GetSprite()->GetCurrentTexCoords();
                ShowInspectedElementImageUI(cur_sprite, Vector2::One * 100.0f, eq_coords);
            }
        }
    }
}

#endif

#pragma endregion
