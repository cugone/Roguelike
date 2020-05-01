#include "Game/Game.hpp"

#include "Engine/Core/ArgumentParser.hpp"
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/KerningFont.hpp"

#include "Engine/Math/Vector2.hpp"
#include "Engine/Math/Vector4.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/Texture.hpp"

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
    _consoleCommands = Console::CommandList(g_theConsole);
    CreateFullscreenConstantBuffer();
    g_theRenderer->RegisterMaterialsFromFolder(std::string{ "Data/Materials" });
    g_theRenderer->RegisterFontsFromFolder(std::string{"Data/Fonts"});
    ingamefont = g_theRenderer->GetFont("TrebuchetMS32");
}

void Game::CreateFullscreenConstantBuffer() {
    _fullscreen_cb = std::unique_ptr<ConstantBuffer>(g_theRenderer->CreateConstantBuffer(&_fullscreen_data, sizeof(_fullscreen_data)));
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::OnEnter_Title() {
    _map.reset(nullptr);
}

void Game::OnEnter_Loading() {
    _done_loading = false;
}

void Game::OnEnter_Main() {
    RegisterCommands();
}

void Game::OnExit_Title() {
    /* DO NOTHING */
}

void Game::OnExit_Loading() {
    /* DO NOTHING */
}

void Game::OnExit_Main() {
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
    _map->BeginFrame();
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
    static Stopwatch text_blinker{ TimeUtils::FPSeconds{0.33f} };
    if(text_blinker.CheckAndReset()) {
        _text_alpha = 1.0f - _text_alpha;
        _text_alpha = std::clamp(_text_alpha, 0.0f, 1.0f);
    }
    if(_done_loading && g_theInputSystem->WasAnyKeyPressed()) {
        ChangeGameState(GameState::Main);
    }
}

void Game::Update_Main(TimeUtils::FPSeconds deltaSeconds) {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        ChangeGameState(GameState::Title);
        return;
    }
    if(g_theApp->LostFocus()) {
        deltaSeconds = TimeUtils::FPSeconds{};
    }
    g_theRenderer->UpdateGameTime(deltaSeconds);
    Camera2D& base_camera = _map->camera;
    HandleDebugInput(base_camera);
    HandlePlayerInput(base_camera);

    UpdateFullscreenEffect(_current_fs_effect);

    base_camera.Update(deltaSeconds);
    _map->Update(deltaSeconds);
}

void Game::Render_Title() const {

    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTarget();
    g_theRenderer->ClearColor(Rgba::Black);
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ ui_view_width, ui_view_height };
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_extents.x, ui_view_half_extents.y };
    auto ui_rightTop = Vector2{ ui_view_half_extents.x, -ui_view_half_extents.y };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = ui_view_half_extents;
    ui_camera.position = ui_cam_pos;
    ui_camera.orientation_degrees = 0.0f;
    ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(ui_camera);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(ui_view_half_extents));
    g_theRenderer->DrawTextLine(ingamefont, "RogueLike");

}

void Game::Render_Loading() const {

    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTarget();
    g_theRenderer->ClearColor(Rgba::Black);
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->SetViewportAsPercent();

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ ui_view_width, ui_view_height };
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_extents.x, ui_view_half_extents.y };
    auto ui_rightTop = Vector2{ ui_view_half_extents.x, -ui_view_half_extents.y };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = ui_view_half_extents;
    ui_camera.position = ui_cam_pos;
    ui_camera.orientation_degrees = 0.0f;
    ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(ui_camera);

    g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(ui_view_half_extents));
    g_theRenderer->DrawTextLine(ingamefont, "LOADING");
    if(_done_loading) {
        const std::string text = "Press Any Key";
        static const auto text_length = ingamefont->CalculateTextWidth(text);
        g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(ui_view_half_extents + Vector2{ text_length * -0.25f, ingamefont->GetLineHeight() }));
        g_theRenderer->DrawTextLine(ingamefont, text, Rgba{255, 255, 255, static_cast<unsigned char>(255.0f * _text_alpha)});
    }

}

void Game::Render_Main() const {

    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTarget(g_theRenderer->GetFullscreenTexture());
    g_theRenderer->ClearColor(Rgba::Black);
    g_theRenderer->ClearDepthStencilBuffer();

    g_theRenderer->SetViewportAsPercent();

    _map->Render(*g_theRenderer);

    if(_debug_render) {
        _map->DebugRender(*g_theRenderer);
    }

    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTargetsToBackBuffer();
    g_theRenderer->ClearColor(Rgba::NoAlpha);
    g_theRenderer->ClearDepthStencilBuffer();
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
        g_theRenderer->DrawQuad2D(Vector2::ZERO, Vector2::ONE, Rgba{ 0, 0, 0, 128 });
    }

    //2D View / HUD
    const float ui_view_height = currentGraphicsOptions.WindowHeight;
    const float ui_view_width = ui_view_height * ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ ui_view_width, ui_view_height };
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_extents.x, ui_view_half_extents.y };
    auto ui_rightTop = Vector2{ ui_view_half_extents.x, -ui_view_half_extents.y };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = ui_view_half_extents;
    ui_camera.position = ui_cam_pos;
    ui_camera.orientation_degrees = 0.0f;
    ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(ui_camera);

    if(g_theApp->LostFocus()) {
        g_theRenderer->SetModelMatrix(Matrix4::CreateTranslationMatrix(ui_view_half_extents));
        g_theRenderer->DrawTextLine(ingamefont, "PAUSED");
    }
}

void Game::EndFrame_Title() {
    /* DO NOTHING */
}

void Game::EndFrame_Loading() {
    if(!_done_loading) {
        LoadUI();
        LoadItems();
        LoadEntities();
        LoadMaps();

        SetCurrentCursorById(CursorId::Green_Box);
        _map->camera.position = _map->CalcMaxDimensions() * 0.5f;
        _done_loading = true;
    }
}

void Game::RegisterCommands() {
    {
        Console::Command moveto{};
        moveto.command_name = "move_to";
        moveto.help_text_short = "Move towards tile";
        moveto.help_text_long = "move_to: Moves towards the highlighted tile";
        moveto.command_function = [this](const std::string& /*args*/) {
            if(_map && _map->player) {
                if(auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
                    _map->player->MoveTo(tile);
                }
            }
        };
        _consoleCommands.AddCommand(moveto);
    }
    {
        Console::Command setvis{};
        setvis.command_name = "set_visibility";
        setvis.help_text_short = "Sets the player's visibility range";
        setvis.help_text_long = "set_visibility [distance]: Sets the player's visibility distance in tiles.";
        setvis.command_function = [this](const std::string& args) {
            ArgumentParser p(args);
            if(_map) {
                float value = 2.0f;
                if(!(p >> value)) {
                    return;
                }
                _map->player->visibility = value;
            }
        };
        _consoleCommands.AddCommand(setvis);
    }
    {
        Console::Command showalltiles{};
        showalltiles.command_name = "showalltiles";
        showalltiles.help_text_short = "Makes every tile visible.";
        showalltiles.help_text_long = "showalltiles [0/1/true/false]: Enables or disables showing every tile. Toggles if no argument.";
        showalltiles.command_function = [this](const std::string& args) {
            ArgumentParser p(args);
            if(_map) {
                static bool value{false};
                if(!(p >> value)) {
                    value = !value;
                }
                const auto maxLayers = _map->GetLayerCount();
                for(std::size_t layerIdx = 0u; layerIdx < maxLayers; ++layerIdx) {
                    if(auto* layer = _map->GetLayer(layerIdx)) {
                        for(auto& tile : *layer) {
                            tile.debug_canSee = value;
                        }
                    }
                }
            }
        };
        _consoleCommands.AddCommand(showalltiles);
    }
    {
        Console::Command setviewheight{};
        setviewheight.command_name = "set_view_height";
        setviewheight.help_text_short = "Sets a layer's view height";
        setviewheight.help_text_long = "set_view height [layer id] [view height in tiles]: Sets the layer's view height in tiles.";
        setviewheight.command_function = [this](const std::string& args) {
            ArgumentParser p(args);
            if(_map) {
                std::size_t id{0u};
                if(p >> id) {
                    if(auto* layer = _map->GetLayer(id)) {
                        float value = 1.0f;
                        if(p >> value) {
                            layer->viewHeight = value;
                            return;
                        }
                        g_theConsole->ErrorMsg("Invalid view height.");
                    }
                }
                g_theConsole->ErrorMsg("Invalid Layer ID.");
            }
        };
        _consoleCommands.AddCommand(setviewheight);
    }
    {
        Console::Command cleartilevis{};
        cleartilevis.command_name = "clear_visibility";
        cleartilevis.help_text_short = "Clears all tile visibility.";
        cleartilevis.help_text_long = "clear_tile_vis: Sets every tile's visibility flags to false.";
        cleartilevis.command_function = [this](const std::string& /*args*/) {
            if(_map) {
                std::size_t layer_index{0u};
                auto* layer = _map->GetLayer(layer_index++);
                for(auto& tile : *layer) {
                    tile.canSee = false;
                    tile.haveSeen = false;
                }
            }
        };
        _consoleCommands.AddCommand(cleartilevis);
    }
    {
        Console::Command color{};
        color.command_name = "set_color";
        color.help_text_short = "Sets entity's color value.";
        color.help_text_long = "set_color [color_value]: Sets an entity's color value. No value returns it to White.";
        color.command_function = [this](const std::string& args) {
            ArgumentParser p{args};
            Entity* entity = nullptr;
            if(_debug_inspected_entity) {
                entity = _debug_inspected_entity;
            }
            if(auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
                if(tile->actor) {
                    entity = tile->actor;
                }
                if(tile->feature) {
                    entity = tile->feature;
                }
            }
            if(!entity) {
                g_theConsole->ErrorMsg("Select an entity to color.");
                return;
            }
            std::string color_str{};
            if(!(p >> color_str)) {
                entity->color = Rgba::White;
                return;
            }
            entity->color = Rgba(color_str);
        };
        _consoleCommands.AddCommand(color);
    }

    {
        Console::Command set_state{};
        set_state.command_name = "set_state";
        set_state.help_text_short = "Sets the state of a feature.";
        set_state.help_text_long = "set_state [name]: Sets the highlighted or selected feature state to [name].";
        set_state.command_function = [this](const std::string& args) {
            ArgumentParser p{args};
            std::string name{};
            if(!(p >> name)) {
                g_theConsole->ErrorMsg("No state name provided.");
                return;
            }
            Entity* entity = nullptr;
            if(_debug_inspected_feature) {
                entity = _debug_inspected_feature;
            } else
            if(auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
                entity = tile->feature;
                if(!entity) {
                    const auto ss = std::string{"Select a feature to set the state to \""} + name + "\".";
                    g_theConsole->ErrorMsg(ss);
                    return;
                }
                if(auto* f = entity->tile->feature) {
                    f->SetState(name);
                }
            }
        };
        _consoleCommands.AddCommand(set_state);
    }

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
            if(auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
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
            }
            if(auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
                if(auto* asActor = dynamic_cast<Actor*>(tile->actor)) {
                    entity = asActor;
                }
            }
            if(!entity) {
                g_theConsole->ErrorMsg("Select an actor to equip.");
                return;
            }
            if(auto* item = entity->inventory.HasItem(item_name)) {
                auto asActor = dynamic_cast<Actor*>(entity);
                if(!asActor) {
                    g_theConsole->ErrorMsg("Entity is not an actor.");
                    return;
                }
                if(auto* equippedItem = asActor->IsEquipped(item->GetEquipSlot())) {
                    asActor->Unequip(equippedItem->GetEquipSlot());
                } else {
                    asActor->Equip(item->GetEquipSlot(), item);
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
                const auto& entities = this->_map->GetEntities();
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

void Game::EndFrame_Main() {
    _map->EndFrame();
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

void Game::LoadUI() {
    LoadCursorsFromFile("Data/Definitions/UI.xml");
}

void Game::LoadMaps() {
    auto str_path = std::string{ "Data/Definitions/Map00.xml" };
    if(FileUtils::IsSafeReadPath(str_path)) {
        if(auto str_buffer = FileUtils::ReadStringBufferFromFile(str_path)) {
            tinyxml2::XMLDocument xml_doc;
            xml_doc.Parse(str_buffer->c_str(), str_buffer->size());
            _map = std::make_unique<Map>(*g_theRenderer, *xml_doc.RootElement());
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
        _cursors.emplace_back(Cursor(*c));
    }
}

void Game::LoadCursorDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        const auto ss = std::string{"Cursor Definitions file at "} + src.string() + " could not be found.";
        ERROR_AND_DIE(ss.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        const auto ss = std::string{"Cursor Definitions at "} + src.string() + " failed to load.";
        ERROR_AND_DIE(ss.c_str());
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
    if(!FS::exists(src)) {
        const auto ss = std::string{"Entities file at "} + src.string() + " could not be found.";
        ERROR_AND_DIE(ss.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        const auto ss = std::string{"Entities source file at "} + src.string() + " could not be loaded.";
        ERROR_AND_DIE(ss.c_str());
    }
    if(auto* xml_entities_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_entities_root, "entities", "definitions,entity", "");
        if(auto* xml_definitions = xml_entities_root->FirstChildElement("definitions")) {
            DataUtils::ValidateXmlElement(*xml_definitions, "definitions", "", "src");
            const auto definitions_src = DataUtils::ParseXmlAttribute(*xml_definitions, "src", std::string{});
            if(definitions_src.empty()) {
                ERROR_AND_DIE("Entity definitions source is empty.");
            }
            FS::path def_src(definitions_src);
            if(!FS::exists(def_src)) {
                ERROR_AND_DIE("Entity definitions source not found.");
            }
            LoadEntityDefinitionsFromFile(def_src);
        }
    }
}

void Game::LoadEntityDefinitionsFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        const auto ss = std::string{"Entity Definitions file at "} + src.string() + " could not be found.";
        ERROR_AND_DIE(ss.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        const auto ss = std::string{"Entity Definitions at "} + src.string() + " failed to load.";
        ERROR_AND_DIE(ss.c_str());
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
    if(!FS::exists(src)) {
        const auto ss = std::string{"Item file at "} + src.string() + " could not be found.";
        ERROR_AND_DIE(ss.c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        const auto ss = std::string{"Item source file at "} +src.string() + " could not be loaded.";
        ERROR_AND_DIE(ss.c_str());
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
    _fullscreen_data.fadeColor = color.GetRgbaAsFloats();
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);

    curFadeTime += g_theRenderer->GetGameFrameTime();
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
    _fullscreen_data.fadeColor = color.GetRgbaAsFloats();
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::FadeOut);
    _fullscreen_cb->Update(*g_theRenderer->GetDeviceContext(), &_fullscreen_data);

    curFadeTime += g_theRenderer->GetGameFrameTime();
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
    _fullscreen_data.gradiantColor = color.GetRgbaAsFloats();
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
    _fullscreen_data.fadeColor = Rgba::Black.GetRgbaAsFloats();
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
    current_cursor = &_cursors[static_cast<std::size_t>(id)];
}

void Game::HandlePlayerInput(Camera2D& base_camera) {
    HandlePlayerKeyboardInput(base_camera);
    HandlePlayerMouseInput(base_camera);
}

void Game::HandlePlayerKeyboardInput(Camera2D& base_camera) {
    const bool is_right = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::D) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::Right) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad6);
    const bool is_left = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::A) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::Left) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad4);

    const bool is_up = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::W) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::Up) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad8);
    const bool is_down = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::S) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::Down) ||
        g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad2);

    const bool is_upright = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad9) || (is_right && is_up);
    const bool is_upleft = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad7) || (is_left && is_up);
    const bool is_downright = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad3) || (is_right && is_down);
    const bool is_downleft = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad1) || (is_left && is_down);

    const bool is_shift = g_theInputSystem->IsKeyDown(KeyCode::Shift);
    const bool is_rest = g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::NumPad5)
        || g_theInputSystem->WasKeyJustPressedOrIsKeyDown(KeyCode::Z);

    if(is_shift) {
        if(is_right) {
            base_camera.position += Vector2::X_AXIS;
        } else if(is_left) {
            base_camera.position += -Vector2::X_AXIS;
        }

        if(is_up) {
            base_camera.position += -Vector2::Y_AXIS;
        } else if(is_down) {
            base_camera.position += Vector2::Y_AXIS;
        }
        if(is_rest) {
            auto player = _map->player;
            _map->FocusEntity(player);
        }
        return;
    }

    auto player = _map->player;
    if(is_rest) {
        player->Act();
        return;
    }
    if(is_upright) {
        _map->MoveOrAttack(player, player->tile->GetNorthEastNeighbor());
    } else if(is_upleft) {
        _map->MoveOrAttack(player, player->tile->GetNorthWestNeighbor());
    } else if(is_downright) {
        _map->MoveOrAttack(player, player->tile->GetSouthEastNeighbor());
    } else if(is_downleft) {
        _map->MoveOrAttack(player, player->tile->GetSouthWestNeighbor());
    } else {
        if(is_right) {
            _map->MoveOrAttack(player, player->tile->GetEastNeighbor());
        } else if(is_left) {
            _map->MoveOrAttack(player, player->tile->GetWestNeighbor());
        }

        if(is_up) {
            _map->MoveOrAttack(player, player->tile->GetNorthNeighbor());
        } else if(is_down) {
            _map->MoveOrAttack(player, player->tile->GetSouthNeighbor());
        }
    }
    _map->FocusEntity(player);
}

void Game::HandlePlayerMouseInput(Camera2D& base_camera) {
    if(g_theInputSystem->WasMouseWheelJustScrolledUp()) {
        DecrementViewHeight();
    }
    if(g_theInputSystem->WasMouseWheelJustScrolledDown()) {
        IncrementViewHeight();
    }
}

void Game::ZoomOut() {
    IncrementViewHeight();
}

void Game::IncrementViewHeight() {
    auto vh = _map->GetLayer(0)->viewHeight;
    vh = std::clamp(++vh, 1.0f, static_cast<float>(_map->GetLayer(0)->tileDimensions.y));
    _map->GetLayer(0)->viewHeight = vh;
}

void Game::ZoomIn() {
    DecrementViewHeight();
}

void Game::DecrementViewHeight() {
    auto vh = _map->GetLayer(0)->viewHeight;
    vh = std::clamp(--vh, 1.0f, static_cast<float>(_map->GetLayer(0)->tileDimensions.y));
    _map->GetLayer(0)->viewHeight = vh;
}

void Game::HandleDebugInput(Camera2D& base_camera) {
    if(_show_debug_window) {
        ShowDebugUI();
    }
    HandleDebugKeyboardInput(base_camera);
    HandleDebugMouseInput(base_camera);
}

void Game::HandleDebugKeyboardInput(Camera2D& base_camera) {
    if(g_theUISystem->GetIO().WantCaptureKeyboard) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::J)) {
        if(!g_theInputSystem->IsMouseLockedToViewport()) {
            g_theInputSystem->LockMouseToViewport(*g_theRenderer->GetOutput()->GetWindow());
        } else {
            g_theInputSystem->UnlockMouseFromViewport();
        }
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F5)) {
        static bool is_fullscreen = false;
        is_fullscreen = !is_fullscreen;
        g_theRenderer->SetFullscreen(is_fullscreen);
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F1)) {
        _show_debug_window = !_show_debug_window;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F2)) {
        _show_grid = !_show_grid;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F3)) {
        _show_world_bounds = !_show_world_bounds;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::F4)) {
        g_theUISystem->ToggleImguiDemoWindow();
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::P)) {
        _map->SetPriorityLayer(static_cast<std::size_t>(MathUtils::GetRandomIntLessThan(static_cast<int>(_map->GetLayerCount()))));
    }

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::B)) {
        base_camera.trauma += 0.2f;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::R)) {
        const auto layer_count = _map->GetLayerCount();
        for(std::size_t i = 0; i < layer_count; ++i) {
            auto* cur_layer = _map->GetLayer(i);
            if(cur_layer) {
                cur_layer->viewHeight = cur_layer->GetDefaultViewHeight();
            }
        }
    }
}

void Game::HandleDebugMouseInput(Camera2D& /*base_camera*/) {
    if(g_theUISystem->GetIO().WantCaptureMouse) {
        return;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::LButton)) {
        const auto& picked_tiles = DebugGetTilesFromMouse();
        _debug_has_picked_tile_with_click = _show_tile_debugger && !picked_tiles.empty();
        _debug_has_picked_entity_with_click = _show_entity_debugger && !picked_tiles.empty();
        _debug_has_picked_feature_with_click = _show_feature_debugger && !picked_tiles.empty();
        if(_debug_has_picked_tile_with_click) {
            _debug_inspected_tiles = picked_tiles;
        }
        if(_debug_has_picked_entity_with_click) {
            _debug_inspected_entity = picked_tiles[0]->actor;
            _debug_has_picked_entity_with_click = _debug_inspected_entity;
        }
        if(_debug_has_picked_feature_with_click) {
            _debug_inspected_feature = picked_tiles[0]->feature;
            _debug_has_picked_feature_with_click = _debug_inspected_feature;
        }
    }
}

void Game::ShowDebugUI() {
    ImGui::SetNextWindowSize(Vector2{ 350.0f, 500.0f }, ImGuiCond_Always);
    if(ImGui::Begin("Debugger", &_show_debug_window, ImGuiWindowFlags_AlwaysAutoResize)) {
        ShowFrameInspectorUI();
        ShowWorldInspectorUI();
        ShowEffectsDebuggerUI();
        ShowTileDebuggerUI();
        ShowFeatureDebuggerUI();
        ShowEntityDebuggerUI(); //Until Tables API is available on master, Entity debugger must be last!
    }
    ImGui::End();
}

void Game::ShowTileDebuggerUI() {
    _show_tile_debugger = ImGui::CollapsingHeader("Tile");
    if(_show_tile_debugger) {
        ShowTileInspectorUI();
    }
}

void Game::ShowEffectsDebuggerUI() {
    _show_effects_debugger = ImGui::CollapsingHeader("Effects");
    if(_show_effects_debugger) {
        ShowEffectsUI();
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
        ImGui::ColorEdit4("Fade In Color##Picker", _fadeIn_color, ImGuiColorEditFlags_NoLabel);
        if(ImGui::InputFloat("Fade In Time (s)", &_debug_fadeInTime)) {
            _fadeInTime = TimeUtils::FPSeconds{ _debug_fadeInTime };
        }
        break;
    case FullscreenEffect::FadeOut:
        ImGui::Text("Effect: Fade Out");
        ImGui::ColorEdit4("Fade Out Color##Picker", _fadeOut_color, ImGuiColorEditFlags_NoLabel);
        if(ImGui::InputFloat("Fade Out Time (s)", &_debug_fadeOutTime)) {
            _fadeOutTime = TimeUtils::FPSeconds{ _debug_fadeOutTime };
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
        ImGui::ColorEdit4("Gradient Color##Picker", _debug_gradientColor, ImGuiColorEditFlags_NoLabel);
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
    _show_entity_debugger = ImGui::CollapsingHeader("Entity");
    if(_show_entity_debugger) {
        ShowEntityInspectorUI();
    }
}

void Game::ShowFeatureDebuggerUI() {
    _show_feature_debugger = ImGui::CollapsingHeader("Feature");
    if(_show_feature_debugger) {
        ShowFeatureInspectorUI();
    }
}

void Game::ShowFrameInspectorUI() {
    static constexpr std::size_t max_histogram_count = 30;
    static std::array<float, max_histogram_count> histogram{};
    static std::size_t histogramIndex = 0;
    static const std::string histogramLabel = "Last " + std::to_string(max_histogram_count) + " Frames";
    if(ImGui::CollapsingHeader("Frame Data")) {
        const auto frameTime = g_theRenderer->GetGameFrameTime().count();
        histogram[histogramIndex++] = frameTime;
        ImGui::Text("FPS: %0.3f", 1.0f / frameTime);
        ImGui::Text("Frame time: %0.3f", frameTime);
        ImGui::PlotHistogram(histogramLabel.c_str(), histogram.data(), static_cast<int>(histogram.size()));
        ImGui::Text("Avg: %0.3f", std::reduce(std::begin(histogram), std::end(histogram), 0.0f) / max_histogram_count);
    }
    histogramIndex %= max_histogram_count;
}

void Game::ShowWorldInspectorUI() {
    if(ImGui::CollapsingHeader("World")) {
        ImGui::Text("Layer 0 view height: %.0f", _map->GetLayer(0)->viewHeight);
        ImGui::Text("Tiles in view: %llu", _map->DebugTilesInViewCount());
        ImGui::Text("Tiles visible in view: %llu", _map->DebugVisibleTilesInViewCount());
        static bool show_grid = false;
        ImGui::Checkbox("World Grid", &show_grid);
        _show_grid = show_grid;
        ImGui::SameLine();
        if(ImGui::ColorEdit4("Grid Color##Picker", _grid_color, ImGuiColorEditFlags_NoLabel)) {
            _map->SetDebugGridColor(_grid_color);
        }
        static bool show_world_bounds = false;
        ImGui::Checkbox("World Bounds", &show_world_bounds);
        _show_world_bounds = show_world_bounds;
        static bool show_all_entities = false;
        ImGui::Checkbox("Show All Entities", &show_all_entities);
        _show_all_entities = show_all_entities;
        static bool show_raycasts = false;
        ImGui::Checkbox("Show raycasts", &show_raycasts);
        _show_raycasts = show_raycasts;
        _debug_render = _show_grid || _show_world_bounds || _show_all_entities || _show_raycasts;
    }
}

void Game::ShowTileInspectorUI() {
    const auto& picked_tiles = DebugGetTilesFromMouse();
    bool has_tile = !picked_tiles.empty();
    bool has_selected_tile = _debug_has_picked_tile_with_click && !_debug_inspected_tiles.empty();
    bool shouldnt_show_inspector = !has_tile && !has_selected_tile;
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
    const auto max_layers = std::size_t{ 9u };
    const auto tiles_per_row = std::size_t{ 3u };
    if(_debug_has_picked_tile_with_click) {
        std::size_t i = 0u;
        for(const auto cur_tile : _debug_inspected_tiles) {
            const auto* cur_def = cur_tile ? cur_tile->GetDefinition() : TileDefinition::GetTileDefinitionByName("void");
            if(const auto* cur_sprite = cur_def->GetSprite()) {
                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(!i || (i % tiles_per_row) < tiles_per_row - 1) {
                    ImGui::SameLine();
                }
            }
            ++i;
        }
        for(; i < max_layers; ++i) {
            const auto* cur_def = TileDefinition::GetTileDefinitionByName("void");
            if(const auto* cur_sprite = cur_def->GetSprite()) {
                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(!i || (i % tiles_per_row) < tiles_per_row - 1) {
                    ImGui::SameLine();
                }
            }
        }
    } else {
        auto picked_count = picked_tiles.size();
        for(std::size_t i = 0; i < max_layers; ++i) {
            const Tile *cur_tile = i < picked_count ? picked_tiles[i] : nullptr;
            const auto* cur_def = cur_tile ? cur_tile->GetDefinition() : TileDefinition::GetTileDefinitionByName("void");
            if(const auto* cur_sprite = cur_def->GetSprite()) {
                const auto tex_coords = cur_sprite->GetCurrentTexCoords();
                const auto dims = Vector2::ONE * 100.0f;
                ImGui::Image(cur_sprite->GetTexture(), dims, tex_coords.mins, tex_coords.maxs, Rgba::White, Rgba::NoAlpha);
                if(!i || (i % tiles_per_row) < tiles_per_row - 1) {
                    ImGui::SameLine();
                }
            }
        }
    }
    ImGui::NewLine();
}

std::vector<Tile*> Game::DebugGetTilesFromMouse() {
    if(g_theUISystem->GetIO().WantCaptureMouse) {
        return {};
    }
    const auto mouse_pos = g_theInputSystem->GetCursorWindowPosition(*g_theRenderer->GetOutput()->GetWindow());
    if(_debug_has_picked_tile_with_click) {
        static std::vector<Tile*> picked_tiles{};
        picked_tiles = _map->PickTilesFromMouseCoords(mouse_pos);
        if(picked_tiles.empty()) {
            return {};
        }
        auto* tile_actor = picked_tiles[0]->actor;
        auto* tile_feature = picked_tiles[0]->feature;
        bool tile_has_entity = !picked_tiles.empty() && (tile_actor || tile_feature);
        if(tile_has_entity && _debug_has_picked_entity_with_click) {
            if(tile_actor) {
                _debug_inspected_entity = tile_actor;
            } else if(tile_feature) {
                _debug_inspected_entity = tile_feature;
            }
        }
        return picked_tiles;
    }
    return _map->PickTilesFromMouseCoords(mouse_pos);
}

void Game::ShowEntityInspectorUI() {
    const auto& picked_tiles = DebugGetTilesFromMouse();
    const auto picked_count = picked_tiles.size();
    bool has_entity = (picked_count > 0 && picked_tiles[0]->actor);
    bool has_selected_entity = _debug_has_picked_entity_with_click && _debug_inspected_entity;
    bool shouldnt_show_inspector = !has_entity && !has_selected_entity;
    if(shouldnt_show_inspector) {
        ImGui::Text("Entity Inspector: None");
        return;
    }
    if(const auto* cur_entity = _debug_inspected_entity ? _debug_inspected_entity : picked_tiles[0]->actor) {
        if(const auto* cur_sprite = cur_entity->sprite) {
            ImGui::Text("Entity Inspector");
            ImGui::SameLine();
            if(ImGui::Button("Unlock Entity")) {
                _debug_has_picked_entity_with_click = false;
                _debug_inspected_entity = nullptr;
            }
            ImGui::Columns(2, "EntityInspectorColumns");
            ShowEntityInspectorEntityColumnUI(cur_entity, cur_sprite);
            ImGui::NextColumn();
            ShowEntityInspectorInventoryColumnUI(cur_entity);
        }
    }
}

void Game::ShowFeatureInspectorUI() {
    const auto& picked_tiles = DebugGetTilesFromMouse();
    const auto picked_count = picked_tiles.size();
    bool has_feature = (picked_count > 0 && picked_tiles[0]->feature);
    bool has_selected_feature = _debug_has_picked_feature_with_click && _debug_inspected_feature;
    bool shouldnt_show_inspector = !has_feature && !has_selected_feature;
    if(shouldnt_show_inspector) {
        ImGui::Text("Feature Inspector: None");
        return;
    }
    if(const auto* cur_entity = _debug_inspected_feature ? _debug_inspected_feature : picked_tiles[0]->feature) {
        if(const auto* cur_sprite = cur_entity->sprite) {
            ImGui::Text("Feature Inspector");
            ImGui::SameLine();
            if(ImGui::Button("Unlock Feature")) {
                _debug_has_picked_feature_with_click = false;
                _debug_inspected_feature = nullptr;
            }
        }
    }
}


void Game::ShowEntityInspectorEntityColumnUI(const Entity* cur_entity, const AnimatedSprite* cur_sprite) {
    std::ostringstream ss;
    ss << "Name: " << cur_entity->name;
    ss << "\nInvisible: " << (cur_entity->def->is_invisible ? "true" : "false");
    ss << "\nAnimated: " << (cur_entity->def->is_animated ? "true" : "false");
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
}
