#include "Game/App.hpp"

#include "Engine/Audio/AudioSystem.hpp"

#include "Engine/Core/Config.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/KeyValueParser.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Profiling/Memory.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Window.hpp"

#include "Engine/UI/UISystem.hpp"

#include "Engine/System/System.hpp"

#include "Game/GameCommon.hpp"
#include "Game/GameConfig.hpp"

#include <algorithm>
#include <condition_variable>
#include <iomanip>

bool CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
EngineMessage GetEngineMessageFromWindowsParams(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

EngineMessage GetEngineMessageFromWindowsParams(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    EngineMessage msg{};
    msg.hWnd = hwnd;
    msg.nativeMessage = uMsg;
    msg.wmMessageCode = EngineSubsystem::GetWindowsSystemMessageFromUintMessage(uMsg);
    msg.wparam = wParam;
    msg.lparam = lParam;
    return msg;
}

bool CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return (g_theSubsystemHead &&
        g_theSubsystemHead->EngineSubsystem::ProcessSystemMessage(
            GetEngineMessageFromWindowsParams(hwnd, uMsg, wParam, lParam)));
}


App::App(const std::string& cmdString)
    : EngineSubsystem()
    , _theJobSystem{std::make_unique<JobSystem>(-1, static_cast<std::size_t>(JobType::Max), new std::condition_variable)}
    , _theFileLogger{std::make_unique<FileLogger>(*_theJobSystem.get(), "game")}
    , _theConfig{std::make_unique<Config>(KeyValueParser{cmdString})}
    , _theRenderer{std::make_unique<Renderer>(*_theJobSystem.get(), *_theFileLogger.get(), *_theConfig.get())}
    , _theConsole{std::make_unique<Console>(*_theFileLogger.get(), *_theRenderer.get())}
    , _theInputSystem{std::make_unique<InputSystem>(*_theFileLogger.get(), *_theRenderer.get())}
    , _theUI{std::make_unique<UISystem>(*_theFileLogger.get(), *_theRenderer.get(), *_theInputSystem.get())}
    , _theAudioSystem{std::make_unique<AudioSystem>(*_theFileLogger.get())}
    , _theGame{std::make_unique<Game>()}
{
    SetupEngineSystemPointers();
    SetupEngineSystemChainOfResponsibility();
    LogSystemDescription();
}

App::~App() {
    g_theSubsystemHead = g_theApp;
}

void App::SetupEngineSystemPointers() {
    g_theJobSystem = _theJobSystem.get();
    g_theFileLogger = _theFileLogger.get();
    g_theConfig = _theConfig.get();
    g_theRenderer = _theRenderer.get();
    g_theUISystem = _theUI.get();
    g_theConsole = _theConsole.get();
    g_theInputSystem = _theInputSystem.get();
    g_theAudioSystem = _theAudioSystem.get();
    g_theGame = _theGame.get();
    g_theApp = this;
}

void App::SetupEngineSystemChainOfResponsibility() {
    g_theConsole->SetNextHandler(g_theUISystem);
    g_theUISystem->SetNextHandler(g_theInputSystem);
    g_theInputSystem->SetNextHandler(g_theRenderer);
    g_theRenderer->SetNextHandler(g_theApp);
    g_theApp->SetNextHandler(nullptr);
    g_theSubsystemHead = g_theConsole;
}

void App::Initialize() {
    g_theConfig->GetValue(std::string{"vsync"}, currentGraphicsOptions.vsync);
    g_theRenderer->Initialize();
    g_theRenderer->SetVSync(currentGraphicsOptions.vsync);
    auto* output = g_theRenderer->GetOutput();
    output->SetTitle("RogueLike");
    output->GetWindow()->custom_message_handler = WindowProc;

    g_theUISystem->Initialize();
    g_theInputSystem->Initialize();
    g_theConsole->Initialize();
    g_theAudioSystem->Initialize();
    g_theGame->Initialize();
}

void App::BeginFrame() {
    g_theJobSystem->BeginFrame();
    g_theUISystem->BeginFrame();
    g_theInputSystem->BeginFrame();
    g_theConsole->BeginFrame();
    g_theAudioSystem->BeginFrame();
    g_theGame->BeginFrame();
    g_theRenderer->BeginFrame();
}

void App::Update(TimeUtils::FPSeconds deltaSeconds) {
    g_theUISystem->Update(deltaSeconds);
    g_theInputSystem->Update(deltaSeconds);
    g_theConsole->Update(deltaSeconds);
    g_theAudioSystem->Update(deltaSeconds);
    g_theGame->Update(deltaSeconds);
    g_theRenderer->Update(deltaSeconds);
}

void App::Render() const {
    g_theGame->Render();
    g_theUISystem->Render();
    g_theConsole->Render();
    g_theAudioSystem->Render();
    g_theInputSystem->Render();
    g_theRenderer->Render();
}

void App::EndFrame() {
    g_theUISystem->EndFrame();
    g_theGame->EndFrame();
    g_theConsole->EndFrame();
    g_theAudioSystem->EndFrame();
    g_theInputSystem->EndFrame();
    g_theRenderer->EndFrame();
}

bool App::ProcessSystemMessage(const EngineMessage& msg) noexcept {

    switch(msg.wmMessageCode) {
    case WindowsSystemMessage::Window_Close:
    {
        SetIsQuitting(true);
        return true;
    }
    case WindowsSystemMessage::Window_Quit:
    {
        SetIsQuitting(true);
        return true;
    }
    case WindowsSystemMessage::Window_Destroy:
    {
        ::PostQuitMessage(0);
        return true;
    }
    case WindowsSystemMessage::Window_ActivateApp:
    {
        WPARAM wp = msg.wparam;
        bool losing_focus = wp == FALSE;
        bool gaining_focus = wp == TRUE;
        if(losing_focus) {
            _current_focus = false;
            _previous_focus = true;
        }
        if(gaining_focus) {
            _current_focus = true;
            _previous_focus = false;
        }
        return true;
    }
    case WindowsSystemMessage::Keyboard_Activate:
    {
        WPARAM wp = msg.wparam;
        auto active_type = LOWORD(wp);
        switch(active_type) {
        case WA_ACTIVE: /* FALLTHROUGH */
        case WA_CLICKACTIVE:
            _current_focus = true;
            _previous_focus = false;
            return true;
        case WA_INACTIVE:
            _current_focus = false;
            _previous_focus = true;
            return true;
        default:
            return false;
        }
    }
    //case WindowsSystemMessage::Window_Size:
    //{
    //    LPARAM lp = msg.lparam;
    //    const auto w = HIWORD(lp);
    //    const auto h = LOWORD(lp);
    //    g_theRenderer->ResizeBuffers();
    //    return true;
    //}
    default:
        return false;
    }
}

bool App::IsQuitting() const {
    return _isQuitting;
}

void App::SetIsQuitting(bool value) {
    _isQuitting = value;
}

void App::RunFrame() {
    using namespace TimeUtils;

    RunMessagePump();

    BeginFrame();

    static FPSeconds previousFrameTime = TimeUtils::GetCurrentTimeElapsed();
    FPSeconds currentFrameTime = TimeUtils::GetCurrentTimeElapsed();
    FPSeconds deltaSeconds = (currentFrameTime - previousFrameTime);
    previousFrameTime = currentFrameTime;

    Update(deltaSeconds);
    Render();
    EndFrame();
    Memory::tick();
}

void App::LogSystemDescription() const {
    auto system = System::GetSystemDesc();
    std::ostringstream ss;
    ss << std::right << std::setfill('-') << std::setw(60) << '\n';
    ss << StringUtils::to_string(system);
    ss << std::right << std::setfill('-') << std::setw(60) << '\n';
    g_theFileLogger->LogLineAndFlush(ss.str());
}

bool App::HasFocus() const {
    return _current_focus;
}

bool App::LostFocus() const {
    return _previous_focus && !_current_focus;
}

bool App::GainedFocus() const {
    return !_previous_focus && _current_focus;
}

void App::RunMessagePump() const {
    MSG msg{};
    for(;;) {
        const BOOL hasMsg = ::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
        if(!hasMsg) {
            break;
        }
        if(!::TranslateAcceleratorA(reinterpret_cast<HWND>(g_theRenderer->GetOutput()->GetWindow()->GetWindowHandle()), reinterpret_cast<HACCEL>(g_theConsole->GetAcceleratorTable()), &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}
