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
#include "Game/EntityDefinition.hpp"
#include "Game/Layer.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Item.hpp"
#include "Game/Inventory.hpp"

Game::~Game() noexcept {
    Item::ClearItemRegistry();
    Actor::ClearActorRegistry();
    EntityDefinition::ClearEntityRegistry();
}

void Game::Initialize() {
    CreateFullscreenConstantBuffer();
    g_theRenderer->RegisterMaterialsFromFolder(std::string{ "Data/Materials" });
    g_theRenderer->RegisterFontsFromFolder(std::string{"Data/Fonts"});
    ingamefont = g_theRenderer->GetFont("TrebuchetMS32");
}

void Game::CreateFullscreenConstantBuffer() {
    _fullscreen_cb = std::unique_ptr<ConstantBuffer>(g_theRenderer->CreateConstantBuffer(&_fullscreen_data, sizeof(_fullscreen_data)));
    _fullscreen_data.resolution = g_theRenderer->GetOutput()->GetDimensions();
    _fullscreen_data.res = Vector2{ _fullscreen_data.resolution.x / 6.0f, _fullscreen_data.resolution.y / 6.0f };
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::OnEnter_Title() {
    /* DO NOTHING */
}

void Game::OnEnter_Loading() {
    _done_loading = false;
}

void Game::OnEnter_Main() {
    /* DO NOTHING */
}

void Game::OnExit_Title() {
    /* DO NOTHING */
}

void Game::OnExit_Loading() {
    /* DO NOTHING */
}

void Game::OnExit_Main() {
    /* DO NOTHING */
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
    if(g_theInputSystem->WasAnyKeyPressed()) {
        ChangeGameState(GameState::Loading);
    }
}

void Game::Update_Loading(TimeUtils::FPSeconds deltaSeconds) {
    static float t = 0.0f;
    t += deltaSeconds.count();
    if(0.33f < t) {
        t = 0.0f;
        _text_alpha = 1.0f - _text_alpha;
        _text_alpha = std::clamp(_text_alpha, 0.0f, 1.0f);
    }
    if(_done_loading && g_theInputSystem->WasAnyKeyPressed()) {
        ChangeGameState(GameState::Main);
    }
}

void Game::Update_Main(TimeUtils::FPSeconds deltaSeconds) {
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::Esc)) {
        g_theApp->SetIsQuitting(true);
        return;
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
    const float ui_view_height = GRAPHICS_OPTION_WINDOW_HEIGHT;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ ui_view_width, ui_view_height };
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_extents.x, ui_view_half_extents.y };
    auto ui_rightTop = Vector2{ ui_view_half_extents.x, -ui_view_half_extents.y };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

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
    const float ui_view_height = GRAPHICS_OPTION_WINDOW_HEIGHT;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ ui_view_width, ui_view_height };
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_extents.x, ui_view_half_extents.y };
    auto ui_rightTop = Vector2{ ui_view_half_extents.x, -ui_view_half_extents.y };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);

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

    g_theRenderer->SetTexture(nullptr); //Force bound texture to invalid.
    g_theRenderer->ResetModelViewProjection();
    g_theRenderer->SetRenderTarget(g_theRenderer->GetTexture("__fullscreen"));
    g_theRenderer->ClearColor(Rgba::Olive);
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

    //2D View / HUD
    const float ui_view_height = GRAPHICS_OPTION_WINDOW_HEIGHT;
    const float ui_view_width = ui_view_height * _ui_camera.GetAspectRatio();
    const auto ui_view_extents = Vector2{ ui_view_width, ui_view_height };
    const auto ui_view_half_extents = ui_view_extents * 0.5f;
    auto ui_leftBottom = Vector2{ -ui_view_half_extents.x, ui_view_half_extents.y };
    auto ui_rightTop = Vector2{ ui_view_half_extents.x, -ui_view_half_extents.y };
    auto ui_nearFar = Vector2{ 0.0f, 1.0f };
    auto ui_cam_pos = ui_view_half_extents;
    _ui_camera.position = ui_cam_pos;
    _ui_camera.orientation_degrees = 0.0f;
    _ui_camera.SetupView(ui_leftBottom, ui_rightTop, ui_nearFar, MathUtils::M_16_BY_9_RATIO);
    g_theRenderer->SetCamera(_ui_camera);
}

void Game::EndFrame_Title() {
    /* DO NOTHING */
}

void Game::EndFrame_Loading() {
    if(!_done_loading) {
        LoadItems();
        LoadEntities();
        LoadMaps();
        _map->camera.position = _map->CalcMaxDimensions() * 0.5f;
        _done_loading = true;
        RegisterCommands();
    }
}

void Game::RegisterCommands() {
    Console::Command give{};
    give.command_name = "give";
    give.help_text_short = "Gives object to entity.";
    give.help_text_long = "give item [count]: adds 1 or [count] item(s) to selected entity's inventory.";
    give.command_function = [this](const std::string& args) {
        ArgumentParser p{ args };
        std::string item_name{};
        if(!p.GetNext(item_name)) {
            g_theConsole->ErrorMsg("No item name provided.");
            return;
        }
        Entity* entity = nullptr;
        int item_count = 1;
        if(_debug_inspected_entity) {
            if(!p.GetNext(item_count)) {
                g_theConsole->WarnMsg("No item count provided. Defaulting to 1.");
            }
            entity = _debug_inspected_entity;
        } else if(auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
            if(!p.GetNext(item_count)) {
                g_theConsole->ErrorMsg("No item count provided. Defaulting to 1.");
            }
            entity = tile->entity;
        }
        entity->inventory.AddStack(item_name, item_count);
    };
    g_theConsole->RegisterCommand(give);

    Console::Command equip{};
    equip.command_name = "equip";
    equip.help_text_short = "Equips/Unequips an item entity's inventory.";
    equip.help_text_long = "equip item: Equips or Unequips an item from/to selected entity.";
    equip.command_function = [this](const std::string& args) {
        ArgumentParser p{ args };
        std::string item_name{};
        if (!p.GetNext(item_name)) {
            return;
        }
        if (auto* tile = _map->PickTileFromMouseCoords(g_theInputSystem->GetMouseCoords(), 0)) {
            if (auto* e = tile->entity) {
                if(auto* i = e->inventory.HasItem(item_name)) {
                    if(auto* asActor = dynamic_cast<Actor*>(e)) {
                        if(auto* q = asActor->IsEquipped(i->GetEquipSlot())) {
                            asActor->Unequip(q->GetEquipSlot());
                        } else {
                            asActor->Equip(i->GetEquipSlot(), i);
                        }
                    }
                } else {
                    std::ostringstream ss;
                    ss << "No " << item_name << " to equip.";
                    g_theConsole->ErrorMsg(ss.str());
                }
            }
        }
    };
    g_theConsole->RegisterCommand(equip);

}

void Game::UnRegisterCommands() {
    g_theConsole->UnregisterCommand("give");
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

void Game::LoadMaps() {
    auto str_path = std::string{ "Data/Definitions/Map00.xml" };
    if(FileUtils::IsSafeReadPath(str_path)) {
        std::string str_buffer{};
        if(FileUtils::ReadBufferFromFile(str_buffer, str_path)) {
            tinyxml2::XMLDocument xml_doc;
            xml_doc.Parse(str_buffer.c_str(), str_buffer.size());
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

void Game::LoadEntitiesFromFile(const std::filesystem::path& src) {
    namespace FS = std::filesystem;
    if(!FS::exists(src)) {
        std::ostringstream ss;
        ss << "Entities file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Entities source file at " << src << " could not be loaded.";
        ERROR_AND_DIE(ss.str().c_str());
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
        std::ostringstream ss;
        ss << "Entity Definitions file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Entity Definitions at " << src << " failed to load.";
        ERROR_AND_DIE(ss.str().c_str());
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
        std::ostringstream ss;
        ss << "Item file at " << src << " could not be found.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    tinyxml2::XMLDocument doc;
    auto xml_result = doc.LoadFile(src.string().c_str());
    if(xml_result != tinyxml2::XML_SUCCESS) {
        std::ostringstream ss;
        ss << "Item source file at " << src << " could not be loaded.";
        ERROR_AND_DIE(ss.str().c_str());
    }
    if(auto* xml_item_root = doc.RootElement()) {
        DataUtils::ValidateXmlElement(*xml_item_root, "items", "spritesheet,item", "");
        auto* xml_item_sheet = xml_item_root->FirstChildElement("spritesheet");
        _item_sheet = g_theRenderer->CreateSpriteSheet(*xml_item_sheet);
        DataUtils::ForEachChildElement(*xml_item_root, "item", [this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "item", "stats,equipslot", "name", "animation", "index");
            ItemBuilder builder{};
            const auto name = DataUtils::ParseXmlAttribute(elem, "name", "UNKNOWN ITEM");
            builder.Name(name);
            builder.Slot(EquipSlotFromString(DataUtils::ParseXmlElementText(*elem.FirstChildElement("equipslot"), "none")));
            if(auto* xml_minstats = elem.FirstChildElement("stats")) {
                builder.MinimumStats(Stats(*xml_minstats));
                builder.MaximumStats(Stats(*xml_minstats));
                if(auto* xml_maxstats = xml_minstats->NextSiblingElement("stats")) {
                    builder.MaximumStats(Stats(*xml_minstats));
                }
            }
            auto startIndex = DataUtils::ParseXmlAttribute(elem, "index", IntVector2::ONE * -1);
            if(auto* xml_animsprite = elem.FirstChildElement("animation")) {
                builder.AnimateSprite(g_theRenderer->CreateAnimatedSprite(this->_item_sheet, *xml_animsprite));
            } else {
                if(startIndex == IntVector2::ONE * -1) {
                    std::ostringstream ss;
                    ss << "Item \"" << name << "\" missing index attribute or animation child element.";
                    ERROR_AND_DIE(ss.str().c_str());
                }
                builder.AnimateSprite(g_theRenderer->CreateAnimatedSprite(this->_item_sheet, startIndex));
            }
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
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);

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
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);

    curFadeTime += g_theRenderer->GetGameFrameTime();
    return _fullscreen_data.fadePercent == 1.0f;
}

void Game::DoScanlines() {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::Scanlines)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Scanlines);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Scanlines);
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::DoLumosity(float brightnessPower /*= 2.4f*/) {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::Lumosity)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Lumosity);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Lumosity);
    _fullscreen_data.lumosityBrightness = brightnessPower;
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);
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
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::DoSepia() {
    static TimeUtils::FPSeconds curFadeTime{};
    if(_fullscreen_data.effectIndex != static_cast<int>(FullscreenEffect::Sepia)) {
        _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Sepia);
        curFadeTime = curFadeTime.zero();
    }
    _fullscreen_data.effectIndex = static_cast<int>(FullscreenEffect::Sepia);
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);
}

void Game::StopFullscreenEffect() {
    static TimeUtils::FPSeconds curFadeTime{};
    _fullscreen_data.effectIndex = -1;
    _fullscreen_data.fadePercent = 0.0f;
    _fullscreen_data.fadeColor = Rgba::Black.GetRgbaAsFloats();
    _fullscreen_cb->Update(g_theRenderer->GetDeviceContext(), &_fullscreen_data);
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
    case FullscreenEffect::Scanlines:
        DoScanlines();
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

void Game::HandlePlayerInput(Camera2D& base_camera) {
    _player_requested_wait = false;
    const bool is_right = g_theInputSystem->WasKeyJustPressed(KeyCode::D) ||
                          g_theInputSystem->WasKeyJustPressed(KeyCode::Right) ||
                          g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad6);
    const bool is_left = g_theInputSystem->WasKeyJustPressed(KeyCode::A) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Left) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad4);

    const bool is_up = g_theInputSystem->WasKeyJustPressed(KeyCode::W) ||
                       g_theInputSystem->WasKeyJustPressed(KeyCode::Up) ||
                       g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad8);
    const bool is_down = g_theInputSystem->WasKeyJustPressed(KeyCode::S) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::Down) ||
                         g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad2);

    const bool is_upright = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad9) || (is_right && is_up);
    const bool is_upleft = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad7) || (is_left && is_up);
    const bool is_downright = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad3) || (is_right && is_down);
    const bool is_downleft = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad1) || (is_left && is_down);

    const bool is_shift = g_theInputSystem->IsKeyDown(KeyCode::Shift);
    const bool is_rest = g_theInputSystem->WasKeyJustPressed(KeyCode::NumPad5)
                         || g_theInputSystem->IsKeyDown(KeyCode::NumPad5)
                         || g_theInputSystem->WasKeyJustPressed(KeyCode::Z)
                         || g_theInputSystem->IsKeyDown(KeyCode::Z);

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
        return;
    }

    if(is_rest) {
        _player_requested_wait = true;
        return;
    }
    auto player = _map->player;
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
    if(!_map->IsEntityInView(player)) {
        _map->FocusEntity(player);
    }

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
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::LeftBracket)) {
        const auto count = _map->GetLayerCount();
        for(std::size_t i = 0; i < count; ++i) {
            auto* cur_layer = _map->GetLayer(i);
            if(cur_layer) {
                ++cur_layer->viewHeight;
            }
        }
    } else if(g_theInputSystem->WasKeyJustPressed(KeyCode::RightBracket)) {
        const auto count = _map->GetLayerCount();
        for(std::size_t i = 0; i < count; ++i) {
            auto* cur_layer = _map->GetLayer(i);
            if(cur_layer) {
                --cur_layer->viewHeight;
            }
        }
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::P)) {
        _map->SetPriorityLayer(static_cast<std::size_t>(MathUtils::GetRandomIntLessThan(static_cast<int>(_map->GetLayerCount()))));
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::O)) {
        _fadeOutTime = TimeUtils::FPSeconds{ 1.0 };
        _current_fs_effect = FullscreenEffect::FadeOut;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::I)) {
        _fadeInTime = TimeUtils::FPSeconds{ 1.0 };
        _current_fs_effect = FullscreenEffect::FadeIn;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::L)) {
        _current_fs_effect = FullscreenEffect::None;
    }
    if(g_theInputSystem->WasKeyJustPressed(KeyCode::K)) {
        _current_fs_effect = FullscreenEffect::Scanlines;
    }

    if(g_theInputSystem->WasKeyJustPressed(KeyCode::B)) {
        base_camera.trauma += 1.0f;
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
        if(_debug_has_picked_tile_with_click) {
            _debug_inspected_tiles = picked_tiles;
        }
        if(_debug_has_picked_entity_with_click) {
            _debug_inspected_entity = picked_tiles[0]->entity;
            _debug_has_picked_entity_with_click = _debug_inspected_entity;
        }
    }
}

void Game::ShowDebugUI() {
    ImGui::SetNextWindowSize(Vector2{ 350.0f, 500.0f }, ImGuiCond_Always);
    if(ImGui::Begin("Debugger", &_show_debug_window, ImGuiWindowFlags_AlwaysAutoResize)) {
        ShowBoundsColoringUI();
        ShowEffectsDebuggerUI();
        ShowTileDebuggerUI();
        ShowEntityDebuggerUI();
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
    bool is_selected = false;
    if(ImGui::BeginCombo("Shader Effect", current_item.c_str())) {
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
        if(ImGui::Selectable("Scanlines")) {
            is_selected = true;
            current_item = "Scanlines";
            _current_fs_effect = FullscreenEffect::Scanlines;
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
    case FullscreenEffect::Scanlines:
        ImGui::Text("Effect: Scanlines");
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
        if(ImGui::DragFloat("Radius", &_fullscreen_data.gradiantRadius, 1.0f, 0.0f, 1.0f)) {
            _fullscreen_data.gradiantRadius = _debug_gradientRadius;
        }
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

void Game::ShowBoundsColoringUI() {
    if(ImGui::CollapsingHeader("World")) {
        ImGui::Checkbox("World Grid", &_show_grid);
        ImGui::SameLine();
        if(ImGui::ColorEdit4("Grid Color##Picker", _grid_color, ImGuiColorEditFlags_NoLabel)) {
            _map->SetDebugGridColor(_grid_color);
        }
        ImGui::Checkbox("World Bounds", &_show_world_bounds);
        ImGui::Checkbox("Show All Entities", &_show_all_entities);
        _debug_render = _show_grid || _show_world_bounds || _show_all_entities;
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
    auto picked_count = picked_tiles.size();
    if(_debug_has_picked_tile_with_click) {
        picked_count = _debug_inspected_tiles.size();
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
        for(i; i < max_layers; ++i) {
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
        if(!_debug_has_picked_tile_with_click) {
            picked_tiles.clear();
        }
        if(_debug_has_picked_tile_with_click) {
            picked_tiles = _map->PickTilesFromMouseCoords(mouse_pos);
        }
        bool tile_has_entity = !picked_tiles.empty() && picked_tiles[0]->entity;
        if(tile_has_entity && _debug_has_picked_entity_with_click) {
            _debug_inspected_entity = picked_tiles[0]->entity;
        }
        return picked_tiles;
    }
    return _map->PickTilesFromMouseCoords(mouse_pos);
}

void Game::ShowEntityInspectorUI() {
    const auto& picked_tiles = DebugGetTilesFromMouse();
    bool has_entity = (!picked_tiles.empty() && picked_tiles[0]->entity);
    bool has_selected_entity = _debug_has_picked_entity_with_click && _debug_inspected_entity;
    bool shouldnt_show_inspector = !has_entity && !has_selected_entity;
    if(shouldnt_show_inspector) {
        ImGui::Text("Entity Inspector: None");
        return;
    }
    if(const auto* cur_entity = _debug_inspected_entity ? _debug_inspected_entity : picked_tiles[0]->entity) {
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
        ss << '\n' << item->GetFriendlyName();
    }
    ImGui::Text(ss.str().c_str());
}
