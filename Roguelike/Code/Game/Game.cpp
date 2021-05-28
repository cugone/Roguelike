#include "Game/Game.hpp"

#include "Engine/Core/ArgumentParser.hpp"
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"
#include "Engine/Core/Utilities.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
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
#include "Game/Tile.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Item.hpp"
#include "Game/Inventory.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <string>

Game::Game()
    :_debug_has_picked_entity_with_click{0}
    , _debug_has_picked_feature_with_click{0}
    , _debug_has_picked_tile_with_click{0}
    , _player_requested_wait{0}
    , _debug_render{0}
    , _show_grid{0}
    , _show_debug_window{0}
    , _show_raycasts{0}
    , _show_world_bounds{0}
    , _show_camera_bounds{0}
    , _show_tile_debugger{0}
    , _show_entity_debugger{0}
    , _show_feature_debugger{0}
    , _show_all_entities{0}
    , _show_camera{0}
    , _show_room_bounds{0}
    , _done_loading{0}
    , _reset_loading_flag{0}
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
    TileDefinition::DestroyTileDefinitions();
}

void Game::Initialize() {
    if(!g_theConfig->AppendFromFile("Data/Config/options.dat")) {
        g_theFileLogger->LogWarnLine("options file not found at Data/Config/options.dat");
    }
    OnMapExit.Subscribe_method(this, &Game::MapExited);
    OnMapEnter.Subscribe_method(this, &Game::MapEntered);

    _consoleCommands = Console::CommandList(g_theConsole);
    CreateFullscreenConstantBuffer();
    g_theRenderer->RegisterMaterialsFromFolder(std::string{"Data/Materials"});
    g_theRenderer->RegisterFontsFromFolder(std::string{"Data/Fonts"});
    ingamefont = g_theRenderer->GetFont("TrebuchetMS32");

    g_theInputSystem->HideMouseCursor();
    //g_theUISystem->RegisterUiWidgetsFromFolder(FileUtils::GetKnownFolderPath(FileUtils::KnownPathID::GameData) / std::string{"UI"});

}

void Game::CreateFullscreenConstantBuffer() {
    _fullscreen_cb = std::unique_ptr<ConstantBuffer>(g_theRenderer->CreateConstantBuffer(&_fullscreen_data, sizeof(_fullscreen_data)));
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::OnEnter_Title() {
    _adventure.reset(nullptr);
}

void Game::OnEnter_Loading() {
    _done_loading = false;
    _reset_loading_flag = false;
    _skip_frame = true;
    g_theUISystem->LoadUiWidget("loading");
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
    _adventure->currentMap->FocusEntity(_adventure->currentMap->player);
    current_cursor->SetCoords(_adventure->currentMap->player->tile->GetCoords());
    g_theInputSystem->LockMouseToWindowViewport();
}

void Game::OnExit_Title() {
    /* DO NOTHING */
}

void Game::OnExit_Loading() {
    g_theUISystem->UnloadUiWidget("loading");
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
    TileDefinition::DestroyTileDefinitions();
}

void Game::BeginFrame_Title() {
    /* DO NOTHING */
}

void Game::BeginFrame_Loading() {
    /* DO NOTHING */
}

void Game::BeginFrame_Main() {
    _adventure->currentMap->BeginFrame();
}

void Game::Update_Title(TimeUtils::FPSeconds /*deltaSeconds*/) {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        g_theApp->SetIsQuitting(true);
        return;
    }
    if(g_theInputSystem->WasAnyKeyPressed()) {
        ChangeGameState(GameState::Loading);
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
    if(g_theApp->LostFocus()) {
        deltaSeconds = TimeUtils::FPSeconds::zero();
    }
    g_theRenderer->UpdateGameTime(deltaSeconds);
    HandleDebugInput();
    HandlePlayerInput();

    UpdateFullscreenEffect(_current_fs_effect);
    _adventure->currentMap->Update(deltaSeconds);
}

void Game::Render_Title() const {

    g_theRenderer->BeginRender();

    g_theRenderer->BeginHUDRender(ui_camera, Vector2::ZERO, currentGraphicsOptions.WindowHeight);

    g_theRenderer->SetModelMatrix(Matrix4::I);
    g_theRenderer->DrawTextLine(ingamefont, "RogueLike");

}

void Game::Render_Loading() const {

    g_theRenderer->BeginRender();

    g_theRenderer->BeginHUDRender(ui_camera, Vector2::ZERO, currentGraphicsOptions.WindowHeight);

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

    g_theRenderer->BeginRender(g_theRenderer->GetFullscreenTexture(), _adventure->currentMap->SkyColor());

    _adventure->currentMap->Render(*g_theRenderer);

    if(_debug_render) {
        _adventure->currentMap->DebugRender(*g_theRenderer);
    }

    g_theRenderer->BeginRenderToBackbuffer(Rgba::NoAlpha);
    g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("Fullscreen"));
    g_theRenderer->SetConstantBuffer(3, _fullscreen_cb.get());
    std::vector<Vertex3D> vbo =
    {
        Vertex3D{Vector3::ZERO}
        ,Vertex3D{Vector3{3.0f, 0.0f, 0.0f}}
        ,Vertex3D{Vector3{0.0f, 3.0f, 0.0f}}
    };
    g_theRenderer->Draw(PrimitiveType::Triangles, vbo, 3);

    if(g_theApp->LostFocus()) {
        g_theRenderer->SetMaterial(g_theRenderer->GetMaterial("__2D"));
        g_theRenderer->DrawQuad2D(Vector2::ZERO, Vector2::ONE, Rgba{0, 0, 0, 128});
    }

    g_theRenderer->BeginHUDRender(ui_camera, Vector2::ZERO, currentGraphicsOptions.WindowHeight);

    if(g_theApp->LostFocus()) {
        g_theRenderer->SetModelMatrix(Matrix4::I);
        g_theRenderer->DrawTextLine(ingamefont, "PAUSED");
    }
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
        SetCurrentCursorById(CursorId::Green_Box);
        _adventure->currentMap->cameraController.GetCamera().position = _adventure->currentMap->CalcMaxDimensions() * 0.5f;
    }
}

void Game::RegisterCommands() {
    {
        Console::Command give{};
        give.command_name = "give";
        give.help_text_short = "Gives object to entity.";
        give.help_text_long = "give item [count]: adds 1 or [count] item(s) to selected entity's inventory.";
        give.command_function = [this](const std::string& args) {
            ArgumentParser p{args};
            std::string item_name{};
            if(!(p >> item_name)) {
                g_theConsole->ErrorMsg("No item name provided.");
                return;
            }
            int item_count = 1;
            p >> item_count;
            Entity* entity = nullptr;
            if(_debug_inspected_entity) {
                entity = _debug_inspected_entity;
            }
            if(auto* tile = _adventure->currentMap->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
                if(auto* asActor = dynamic_cast<Actor*>(tile->actor)) {
                    entity = asActor;
                }
            }
            if(!entity) {
                g_theConsole->ErrorMsg("Select an actor to give the item to.");
                return;
            }
            auto* item = Item::GetItem(item_name);
            if(!item) {
                const auto ss = std::string{"Item "} + item_name + " not found.";
                g_theConsole->ErrorMsg(ss);
                return;
            }
            entity->inventory.AddStack(item_name, item_count);
            auto* asActor = dynamic_cast<Actor*>(entity);
            if(!asActor) {
                g_theConsole->ErrorMsg("Entity is not an actor.");
                return;
            }
            if(!asActor->IsEquipped(item->GetEquipSlot())) {
                asActor->Equip(item->GetEquipSlot(), item);
            }
        };
        _consoleCommands.AddCommand(give);
    }

    {
        Console::Command equip{};
        equip.command_name = "equip";
        equip.help_text_short = "Equips/Unequips an item in an actor's inventory.";
        equip.help_text_long = "equip item: Equips or Unequips an item from/to selected actor.";
        equip.command_function = [this](const std::string& args) {
            ArgumentParser p{args};
            std::string item_name{};
            if(!(p >> item_name)) {
                return;
            }
            Entity* entity = nullptr;
            if(_debug_inspected_entity) {
                entity = _debug_inspected_entity;
            } else {
                if(auto* tile = _adventure->currentMap->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
                    if(auto* asActor = dynamic_cast<Actor*>(tile->actor)) {
                        entity = asActor;
                    }
                }
            }
            if(!entity) {
                g_theConsole->ErrorMsg("Select an actor to equip.");
                return;
            }
            if(auto* item = entity->inventory.HasItem(item_name)) {
                if(auto asActor = dynamic_cast<Actor*>(entity); !asActor) {
                    g_theConsole->ErrorMsg("Entity is not an actor.");
                    return;
                } else {
                    if(auto* equippedItem = asActor->IsEquipped(item->GetEquipSlot())) {
                        asActor->Unequip(equippedItem->GetEquipSlot());
                    } else {
                        asActor->Equip(item->GetEquipSlot(), item);
                    }
                }
            } else {
                const auto ss = std::string{"Actor does not have "} + item_name;
                g_theConsole->ErrorMsg(ss);
            }
        };
        _consoleCommands.AddCommand(equip);
    }

    {
        Console::Command list{};
        list.command_name = "list";
        list.help_text_short = "list [items|actors|features|all]";
        list.help_text_long = "lists all entities of specified type or all.";
        list.command_function = [this, list](const std::string& args) {
            ArgumentParser p{args};
            std::string subcommand{};
            if(!(p >> subcommand)) {
                g_theConsole->PrintMsg(list.help_text_short);
                return;
            }
            subcommand = StringUtils::ToLowerCase(StringUtils::TrimWhitespace(subcommand));
            const auto all_item_names = [=]()->std::vector<std::string> {
                std::vector<std::string> names{};
                names.reserve(Item::s_registry.size());
                std::for_each(std::cbegin(Item::s_registry), std::cend(Item::s_registry), [&](const auto& iter) { names.push_back(iter.first); });
                return names;
            }();
            const auto all_actor_names = [=]()->std::vector<std::string> {
                const auto& entities = this->_adventure->currentMap->GetEntities();
                std::vector<std::string> names{};
                names.reserve(entities.size());
                std::for_each(std::cbegin(entities), std::cend(entities), [&](const Entity* e) { if(const Actor* a = dynamic_cast<const Actor*>(e)) names.push_back(a->name); });
                return names;
            }();
            //const auto all_feature_names = [=]()->std::vector<std::string> {
            //    const auto& entities = this->_map->GetEntities();
            //    std::vector<std::string> names{};
            //    names.reserve(entities.size());
            //    std::for_each(std::cbegin(entities), std::cend(entities), [&](const Entity* e) { if(const Feature* f = dynamic_cast<const Feature*>(e)) names.push_back(f->name); });
            //    return names;
            //}();
            const auto all_feature_names = std::vector<std::string>{};
            if(subcommand == "items") {
                for(const auto& name : all_item_names) {
                    g_theConsole->PrintMsg(name);
                }
            } else if(subcommand == "actors") {
                for(const auto& name : all_actor_names) {
                    g_theConsole->PrintMsg(name);
                }
            } else if(subcommand == "features") {
                g_theConsole->WarnMsg("Not yet implemented.");
                for(const auto& name : all_feature_names) {
                    g_theConsole->PrintMsg(name);
                }
            } else if(subcommand == "all") {
                for(const auto& name : all_item_names) {
                    g_theConsole->PrintMsg(name);
                }
                for(const auto& name : all_actor_names) {
                    g_theConsole->PrintMsg(name);
                }
                for(const auto& name : all_feature_names) {
                    g_theConsole->PrintMsg(name);
                }
            } else {
                g_theConsole->WarnMsg("Invalid argument.");
                g_theConsole->WarnMsg(list.help_text_short);
            }
        };
        _consoleCommands.AddCommand(list);
    }


    g_theConsole->PushCommandList(_consoleCommands);
}

void Game::UnRegisterCommands() {
    g_theConsole->PopCommandList(_consoleCommands);
}

void Game::MapEntered() noexcept {
    static bool requested_enter = false;
    if(_adventure->currentMap->IsPlayerOnEntrance()) {
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
    if(_adventure->currentMap->IsPlayerOnExit()) {
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
    _adventure->currentMap->EndFrame();
    OnMapExit.Trigger();
    OnMapEnter.Trigger();
}

void Game::ChangeGameState(const GameState& newState) {
    _nextGameState = newState;
}

void Game::OnEnterState(const GameState& state) {
    switch(state) {
    case GameState::Title:    OnEnter_Title();   break;
    case GameState::Loading:  OnEnter_Loading(); break;
    case GameState::Main:     OnEnter_Main();    break;
    default: ERROR_AND_DIE("ON ENTER UNDEFINED GAME STATE") break;
    }
}

void Game::OnExitState(const GameState& state) {
    switch(state) {
    case GameState::Title:    OnExit_Title();   break;
    case GameState::Loading:  OnExit_Loading(); break;
    case GameState::Main:     OnExit_Main();    break;
    default: ERROR_AND_DIE("ON ENTER UNDEFINED GAME STATE") break;
    }
}

const bool Game::IsDebugWindowOpen() const noexcept {
#ifdef PROFILE_BUILD
    return _show_debug_window;
#else
    return false;
#endif
}

void Game::LoadUI() {
    LoadCursorsFromFile("Data/Definitions/UI.xml");
}

void Game::LoadMaps() {
    //auto str_path = std::string{"Data/Definitions/Map01.xml"};
    auto str_path = std::string{"Data/Definitions/Adventure.xml"};
    if(FileUtils::IsSafeReadPath(str_path)) {
        if(auto str_buffer = FileUtils::ReadStringBufferFromFile(str_path)) {
            tinyxml2::XMLDocument xml_doc;
            if(auto parse_result = xml_doc.Parse(str_buffer->c_str(), str_buffer->size()); parse_result == tinyxml2::XML_SUCCESS) {
                _adventure = std::make_unique<Adventure>(*g_theRenderer, *xml_doc.RootElement());
            }
        }
    }
}

void Game::LoadEntities() {
    LoadEntitiesFromFile("Data/Definitions/Entities.xml");
}

void Game::LoadItems() {
    LoadItemsFromFile("Data/Definitions/Items.xml");
}

void Game::LoadCursorsFromFile(const std::filesystem::path& src) {
    LoadCursorDefinitionsFromFile(src);
    for(const auto& c : CursorDefinition::GetLoadedDefinitions()) {
        _cursors.emplace_back(*c);
    }
}

void Game::LoadCursorDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    {
        const auto error_msg = std::string{"Cursor Definitions file at "} + src.string() + " could not be found.";
        GUARANTEE_OR_DIE(FS::exists(src), error_msg.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    {
        const auto error_msg = std::string{"Cursor Definitions at "} + src.string() + " failed to load.";
        GUARANTEE_OR_DIE(xml_result == tinyxml2::XML_SUCCESS, error_msg.c_str());
    }
    if(auto* xml_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_root, "UI", "spritesheet", "", "cursors,overlays");
        auto* xml_spritesheet = xml_root->FirstChildElement("spritesheet");
        _cursor_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet);
        if(auto* xml_cursors = xml_root->FirstChildElement("cursors")) {
            DataUtils::ForEachChildElement(*xml_cursors, "cursor",
                [this](const XMLElement& elem) {
                    CursorDefinition::CreateCursorDefinition(*g_theRenderer, elem, _cursor_sheet);
                });
        }
    }
}

void Game::LoadEntitiesFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    {
        const auto error_msg = std::string{"Entities file at "} + src.string() + " could not be found.";
        GUARANTEE_OR_DIE(FS::exists(src), error_msg.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    {
        const auto error_msg = std::string{"Entities source file at "} + src.string() + " could not be loaded.";
        GUARANTEE_OR_DIE(xml_result == tinyxml2::XML_SUCCESS, error_msg.c_str());
    }

    if(auto* xml_entities_root = doc.RootElement()) {
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

void Game::LoadEntityDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    {
        const auto error_msg = std::string{"Entity Definitions file at "} + src.string() + " could not be found.";
        GUARANTEE_OR_DIE(FS::exists(src), error_msg.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    {
        const auto error_msg = std::string{"Entity Definitions at "} + src.string() + " failed to load.";
        GUARANTEE_OR_DIE(xml_result == tinyxml2::XML_SUCCESS, error_msg.c_str());
    }
    if(auto* xml_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_root, "entityDefinitions", "spritesheet,entityDefinition", "");
        auto* xml_spritesheet = xml_root->FirstChildElement("spritesheet");
        _entity_sheet = g_theRenderer->CreateSpriteSheet(*xml_spritesheet);
        DataUtils::ForEachChildElement(*xml_root, "entityDefinition",
            [this](const XMLElement& elem) {
                EntityDefinition::CreateEntityDefinition(*g_theRenderer, elem, _entity_sheet);
            });
    }
}

void Game::LoadItemsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    {
        const auto error_msg = std::string{"Item file at "} + src.string() + " could not be found.";
        GUARANTEE_OR_DIE(FS::exists(src), error_msg.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    {
        const auto error_msg = std::string{"Item source file at "} + src.string() + " could not be loaded.";
        GUARANTEE_OR_DIE(xml_result == tinyxml2::XML_SUCCESS, error_msg.c_str());
    }
    if(auto* xml_item_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_item_root, "items", "spritesheet,item", "");
        auto* xml_item_sheet = xml_item_root->FirstChildElement("spritesheet");
        _item_sheet = g_theRenderer->CreateSpriteSheet(*xml_item_sheet);
        DataUtils::ForEachChildElement(*xml_item_root, "item", [this](const XMLElement& elem) {
            ItemBuilder builder(elem, _item_sheet);
            builder.Build();
            });
    }
}

void Game::BeginFrame() {
    if(_nextGameState != _currentGameState) {
        OnExitState(_currentGameState);
        _currentGameState = _nextGameState;
        OnEnterState(_currentGameState);
    }
    switch(_currentGameState) {
    case GameState::Title:   BeginFrame_Title(); break;
    case GameState::Loading: BeginFrame_Loading(); break;
    case GameState::Main:    BeginFrame_Main(); break;
    default:                 ERROR_AND_DIE("BEGIN FRAME UNDEFINED GAME STATE"); break;
    }
}

void Game::Update(TimeUtils::FPSeconds deltaSeconds) {
    switch(_currentGameState) {
    case GameState::Title:   Update_Title(deltaSeconds); break;
    case GameState::Loading: Update_Loading(deltaSeconds); break;
    case GameState::Main:    Update_Main(deltaSeconds); break;
    default:                 ERROR_AND_DIE("UPDATE UNDEFINED GAME STATE"); break;
    }
}

bool Game::DoFadeIn(const Rgba& color, TimeUtils::FPSeconds fadeTime) {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::FadeIn)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::FadeIn);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.fadePercent = curFadeTime / fadeTime;
    _fullscreen_data.fadePercent = std::clamp(_fullscreen_data.fadePercent, 0.0f, 1.0f);
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::FadeIn);
    const auto [r, g, b, a] = color.GetAsFloats();
    _fullscreen_data.fadeColor = Vector4{r, g, b, a};
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);

    curFadeTime += g_theRenderer->GetGameFrameTime();
    if(_fullscreen_data.fadePercent == 1.0f) {
        _fullscreen_callback();
    }
    return _fullscreen_data.fadePercent == 1.0f;
}

bool Game::DoFadeOut(const Rgba& color, TimeUtils::FPSeconds fadeTime) {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::FadeOut)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::FadeOut);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.fadePercent = curFadeTime / fadeTime;
    _fullscreen_data.fadePercent = std::clamp(_fullscreen_data.fadePercent, 0.0f, 1.0f);
    const auto [r, g, b, a] = color.GetAsFloats();
    _fullscreen_data.fadeColor = Vector4{r, g, b, a};
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::FadeOut);
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);

    curFadeTime += g_theRenderer->GetGameFrameTime();
    if(_fullscreen_data.fadePercent == 1.0f) {
        _fullscreen_callback();
    }
    return _fullscreen_data.fadePercent == 1.0f;
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
    _fullscreen_data.fadeColor = Vector4::W_AXIS;
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

void Game::Render() const {
    switch(_currentGameState) {
    case GameState::Title:   Render_Title(); break;
    case GameState::Loading: Render_Loading(); break;
    case GameState::Main:    Render_Main(); break;
    default:                 ERROR_AND_DIE("RENDER UNDEFINED GAME STATE"); break;
    }
}

void Game::EndFrame() {
    g_theRenderer->SetVSync(currentGraphicsOptions.vsync);
    switch(_currentGameState) {
    case GameState::Title:   EndFrame_Title(); break;
    case GameState::Loading: EndFrame_Loading(); break;
    case GameState::Main:    EndFrame_Main(); break;
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

void Game::HandlePlayerInput() {
    HandlePlayerKeyboardInput();
    //HandlePlayerControllerInput();
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
            _adventure->currentMap->cameraController.Translate(Vector2::X_AXIS);
        } else if(is_left_held) {
            _adventure->currentMap->cameraController.Translate(-Vector2::X_AXIS);
        }

        if(is_up_held) {
            _adventure->currentMap->cameraController.Translate(-Vector2::Y_AXIS);
        } else if(is_down_held) {
            _adventure->currentMap->cameraController.Translate(Vector2::Y_AXIS);
        }
        return;
    }

    auto player = _adventure->currentMap->player;
    if(is_rest) {
        player->Act();
        return;
    }
    if(is_upright) {
        _adventure->currentMap->MoveOrAttack(player, player->tile->GetNorthEastNeighbor());
    } else if(is_upleft) {
        _adventure->currentMap->MoveOrAttack(player, player->tile->GetNorthWestNeighbor());
    } else if(is_downright) {
        _adventure->currentMap->MoveOrAttack(player, player->tile->GetSouthEastNeighbor());
    } else if(is_downleft) {
        _adventure->currentMap->MoveOrAttack(player, player->tile->GetSouthWestNeighbor());
    } else {
        if(is_right) {
            _adventure->currentMap->MoveOrAttack(player, player->tile->GetEastNeighbor());
        } else if(is_left) {
            _adventure->currentMap->MoveOrAttack(player, player->tile->GetWestNeighbor());
        }

        if(is_up) {
            _adventure->currentMap->MoveOrAttack(player, player->tile->GetNorthNeighbor());
        } else if(is_down) {
            _adventure->currentMap->MoveOrAttack(player, player->tile->GetSouthNeighbor());
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
    rthumb.y *= currentGraphicsOptions.InvertMouseY ? 1.0f : -1.0f;
    _adventure->currentMap->cameraController.Translate(_cam_speed * rthumb * g_theRenderer->GetGameFrameTime().count());

    if(controller.WasButtonJustPressed(XboxController::Button::RightThumb)) {
        _adventure->currentMap->FocusEntity(_adventure->currentMap->player);
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
    _adventure->currentMap->ZoomOut();
}

void Game::ZoomIn() {
    _adventure->currentMap->ZoomIn();
}

void Game::HandleDebugInput() {
#ifdef PROFILE_BUILD
    if(_show_debug_window) {
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
    if(!_show_debug_window && !g_theUISystem->IsAnyImguiDebugWindowVisible()) {
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
        _show_debug_window = !_show_debug_window;
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
        _adventure->currentMap->SetPriorityLayer(static_cast<std::size_t>(MathUtils::GetRandomIntLessThan(static_cast<int>(_adventure->currentMap->GetLayerCount()))));
    }

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::G)) {
        _adventure->currentMap->RegenerateMap();
    }

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::B)) {
        _adventure->currentMap->ShakeCamera([]()->float { const auto t = g_theRenderer->GetGameTime().count(); return std::cos(t) * std::sin(t); });
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
        _debug_has_picked_tile_with_click = _show_tile_debugger && picked_tiles.has_value();
        _debug_has_picked_entity_with_click = _show_entity_debugger && picked_tiles.has_value();
        _debug_has_picked_feature_with_click = _show_feature_debugger && picked_tiles.has_value();
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
        if(const auto* tile = _adventure->currentMap->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile != nullptr) {
            _adventure->currentMap->player->SetPosition(tile->GetCoords());
        }
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::MButton)) {
        if(auto* tile = _adventure->currentMap->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0); tile != nullptr) {
            tile->SetEntity(Feature::GetFeatureByName("flamePedestal"));
        }
    }
#endif
}

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
    if(_show_tile_debugger = ImGui::BeginTabItem("Tile", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder); _show_tile_debugger) {
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
    if(_show_entity_debugger = ImGui::BeginTabItem("Entity", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder); _show_entity_debugger) {
        ShowEntityInspectorUI();
        ImGui::EndTabItem();
    }
}

void Game::ShowFeatureDebuggerUI() {
    if(_show_feature_debugger = ImGui::BeginTabItem("Feature", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder); _show_feature_debugger) {
        ShowFeatureInspectorUI();
        ImGui::EndTabItem();
    }
}

void Game::ShowFrameInspectorUI() {
    static constexpr std::size_t max_histogram_count = 60;
    static std::array<float, max_histogram_count> histogram{};
    static std::size_t histogramIndex = 0;
    static const std::string histogramLabel = "Last " + std::to_string(max_histogram_count) + " Frames";
    if(ImGui::BeginTabItem("Frame Data", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder)) {
        const auto frameTime = g_theRenderer->GetGameFrameTime().count();
        histogram[histogramIndex++] = frameTime;
        ImGui::Text("FPS: %0.1f", 1.0f / frameTime);
        ImGui::PlotHistogram(histogramLabel.c_str(), histogram.data(), static_cast<int>(histogram.size()));
        ImGui::Text("Frame time: %0.7f", frameTime);
        ImGui::Text("Min: %0.7f", *std::min_element(std::begin(histogram), std::end(histogram)));
        ImGui::Text("Max: %0.7f", *std::max_element(std::begin(histogram), std::end(histogram)));
        ImGui::Text("Avg: %0.7f", std::reduce(std::begin(histogram), std::end(histogram), 0.0f) / max_histogram_count);
        ImGui::Checkbox("Vsync", &currentGraphicsOptions.vsync);
        if(ImGui::Button("Take Screenshot")) {
            RequestScreenShot();
        }
        ImGui::EndTabItem();
    }
    histogramIndex %= max_histogram_count;
}

void Game::ShowWorldInspectorUI() {
    if(ImGui::BeginTabItem("World", nullptr, ImGuiTabItemFlags_NoCloseWithMiddleMouseButton | ImGuiTabItemFlags_NoReorder)) {
        ImGui::Text("View height: %.0f", _adventure->currentMap->cameraController.GetCamera().GetViewHeight());
        ImGui::Text("Camera: [%.1f,%.1f]", _adventure->currentMap->cameraController.GetCamera().position.x, _adventure->currentMap->cameraController.GetCamera().position.y);
        ImGui::Text("Tiles in view: %llu", _adventure->currentMap->DebugTilesInViewCount());
        ImGui::Text("Tiles visible in view: %llu", _adventure->currentMap->DebugVisibleTilesInViewCount());
        static bool show_camera = false;
        ImGui::Checkbox("Show Camera", &show_camera);
        _show_camera = show_camera;
        static bool show_grid = false;
        ImGui::Checkbox("World Grid", &show_grid);
        _show_grid = show_grid;
        ImGui::SameLine();
        if(ImGui::ColorEdit4("Grid Color##Picker", _grid_color, ImGuiColorEditFlags_NoLabel)) {
            _adventure->currentMap->SetDebugGridColor(_grid_color);
        }
        static bool show_world_bounds = false;
        ImGui::Checkbox("World Bounds", &show_world_bounds);
        _show_world_bounds = show_world_bounds;
        static bool show_camera_bounds = false;
        ImGui::Checkbox("Camera Bounds", &show_camera_bounds);
        _show_camera_bounds = show_camera_bounds;
        static bool show_room_bounds = false;
        ImGui::Checkbox("Show Room Bounds", &show_room_bounds);
        _show_room_bounds = show_room_bounds;
        static bool show_all_entities = false;
        ImGui::Checkbox("Show All Entities", &show_all_entities);
        _show_all_entities = show_all_entities;
        static bool show_raycasts = false;
        ImGui::Checkbox("Show raycasts", &show_raycasts);
        _show_raycasts = show_raycasts;
        _debug_render = _show_room_bounds || _show_camera || _show_grid || _show_world_bounds || _show_camera_bounds || _show_all_entities || _show_raycasts;
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
        if(ImGui::Button("Unlock Tile")) {
            _debug_has_picked_tile_with_click = false;
            _debug_inspected_tiles.clear();
        }
        const auto max_layers = std::size_t{9u};
        const auto tiles_per_row = std::size_t{3u};
        const auto tiles_per_col = std::size_t{3u};
        const auto debugger_window_content_region_width = ImGui::GetWindowContentRegionWidth();
        const auto debugger_window_width = ImGui::GetWindowWidth();
        //TODO: Get Centering working. Not currently in API.
        ImGui::SetNextItemWidth(debugger_window_content_region_width * 0.5f);
        if(ImGui::BeginTable("TileInspectorTable", tiles_per_col, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX)) {
            ImGui::TableSetupColumn("TileInspectorTableLeftColumn", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("TileInspectorTableCenterColumn", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("TileInspectorTableRightColumn", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            if(_debug_has_picked_tile_with_click) {
                for(auto row = std::size_t{0u}; row != tiles_per_row; ++row) {
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 100.0f);
                    for(auto col = std::size_t{0u}; col != tiles_per_col; ++col) {
                        ImGui::TableSetColumnIndex(static_cast<int>(col));
                        const auto index = row * tiles_per_col + col;
                        if(index >= _debug_inspected_tiles.size()) {
                            continue;
                        }
                        if(const auto* cur_tile = _debug_inspected_tiles[index]; cur_tile == nullptr) {
                            continue;
                        } else {
                            const auto* cur_def = TileDefinition::GetTileDefinitionByName(cur_tile->GetType());
                            if(cur_def == nullptr) {
                                cur_def = TileDefinition::GetTileDefinitionByName("void");
                            }
                            if(const auto* cur_sprite = cur_def->GetSprite()) {
                                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                                const auto dims = Vector2::ONE * 100.0f;
                                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                                if(ImGui::IsItemHovered()) {
                                    ImGui::BeginTooltip();
                                    ShowTileInspectorStatsTableUI(cur_def, cur_tile);
                                    ImGui::EndTooltip();
                                }
                            }
                        }
                    }
                }
            } else {
                for(auto row = std::size_t{0u}; row != tiles_per_row; ++row) {
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, 100.0f);
                    for(auto col = std::size_t{0u}; col != tiles_per_col; ++col) {
                        ImGui::TableSetColumnIndex(static_cast<int>(col));
                        const auto index = row * tiles_per_col + col;
                        if(index >= picked_tiles->size()) {
                            continue;
                        }
                        const auto* cur_tile = (*picked_tiles)[index];
                        const auto* cur_def = cur_tile ? TileDefinition::GetTileDefinitionByName(cur_tile->GetType()) : TileDefinition::GetTileDefinitionByName("void");
                        if(const auto* cur_sprite = cur_def->GetSprite()) {
                            const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                            const auto dims = Vector2::ONE * 100.0f;
                            ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    }
}

void Game::ShowTileInspectorStatsTableUI(const TileDefinition* cur_def, const Tile* cur_tile) {
    if(ImGui::BeginTable("TileInspectorTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("TileInspectorTableRowIdsColumn");
        ImGui::TableSetupColumn("TileInspectorTableRowValuesColumn");
        //ImGui::TableHeadersRow(); //Commented out to hide header row.
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

        //ImGui::TableNextRow();
        //ImGui::TableNextColumn();
        //ImGui::Text("Have Seen:");
        //ImGui::TableNextColumn();
        //ImGui::Text((cur_tile && cur_tile->haveSeen ? "true" : "false"));

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
}

std::optional<std::vector<Tile*>> Game::DebugGetTilesFromMouse() {
    if(g_theUISystem->WantsInputMouseCapture()) {
        return {};
    }
    const auto mouse_pos = g_theInputSystem->GetCursorWindowPosition(*g_theRenderer->GetOutput()->GetWindow());
    if(_debug_has_picked_tile_with_click) {
        static std::optional<std::vector<Tile*>> picked_tiles{};
        if(picked_tiles = _adventure->currentMap->PickTilesFromMouseCoords(mouse_pos); !picked_tiles.has_value()) {
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
    return _adventure->currentMap->PickTilesFromMouseCoords(mouse_pos);
}

std::optional<std::vector<Tile*>> Game::DebugGetTilesFromCursor() {
    if(!current_cursor) {
        return {};
    }
    if(_debug_has_picked_tile_with_click) {
        static std::optional<std::vector<Tile*>> picked_tiles{};
        if(picked_tiles = _adventure->currentMap->PickTilesFromWorldCoords(Vector2{current_cursor->GetCoords()}); !picked_tiles.has_value()) {
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
    return _adventure->currentMap->PickTilesFromWorldCoords(Vector2{current_cursor->GetCoords()});
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
        if(const auto* cur_entity = _debug_inspected_entity ? _debug_inspected_entity : (*picked_tiles)[0]->actor) {
            if(const auto* cur_sprite = cur_entity->sprite) {
                ImGui::Text("Entity Inspector");
                ImGui::SameLine();
                if(ImGui::Button("Unlock Entity")) {
                    _debug_has_picked_entity_with_click = false;
                    _debug_inspected_entity = nullptr;
                }
                if(ImGui::BeginTable("EntityInspectorTable", 2, ImGuiTableFlags_Borders)) {
                    ImGui::TableSetupColumn("EntityInspectorTableEntityColumn");
                    ImGui::TableSetupColumn("EntityInspectorTableInventoryColumn");
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
                if(ImGui::Button("Unlock Feature")) {
                    _debug_has_picked_feature_with_click = false;
                    _debug_inspected_feature = nullptr;
                }
                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(const auto* actor = dynamic_cast<const Actor*>(cur_entity)) {
                    for(const auto& eq : actor->GetEquipment()) {
                        if(eq) {
                            const auto eq_coords = eq->GetSprite()->GetCurrentTexCoords();
                            ImGui::SameLine(8.0f);
                            ImGui::Image(cur_sprite->GetTexture(), dims, eq_coords.mins, eq_coords.maxs, Rgba::White, Rgba::NoAlpha);
                        }
                    }
                }
            }
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
    const auto tex_coords = cur_sprite->GetCurrentTexCoords();
    const auto dims = Vector2::ONE * 100.0f;
    ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
    if(const auto* actor = dynamic_cast<const Actor*>(cur_entity)) {
        for(const auto& eq : actor->GetEquipment()) {
            if(eq) {
                const auto eq_coords = eq->GetSprite()->GetCurrentTexCoords();
                ImGui::SameLine(8.0f);
                ImGui::Image(cur_sprite->GetTexture(), dims, eq_coords.mins, eq_coords.maxs, Rgba::White, Rgba::NoAlpha);
            }
        }
    }
}

void Game::ShowEntityInspectorInventoryColumnUI(const Entity* cur_entity) {
    if(!cur_entity) {
        return;
    }
    std::ostringstream ss;
    if(cur_entity->inventory.empty()) {
        ss << "Inventory: None";
        ImGui::Text(ss.str().c_str());
        return;
    }
    ss << "Inventory:";
    for(const auto* item : cur_entity->inventory) {
        const auto count = item->GetCount();
        ss << '\n' << item->GetFriendlyName();
        if(count > 1) {
            ss << " x" << (count > 99u ? 99u : count);
        }
    }
    ImGui::Text(ss.str().c_str());
    ImGui::SameLine();
    if(const auto* asActor = dynamic_cast<const Actor*>(cur_entity); asActor) {
        if(ImGui::SmallButton("Equip")) {
            g_theConsole->RunCommand(std::string{"Equip "} + ss.str());
        }
    }
}

#endif
