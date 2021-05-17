
#include "Engine/Core/BuildConfig.hpp"
#include "Engine/Core/Config.hpp"
#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/KeyValueParser.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Win.hpp"

#include "Game/GameCommon.hpp"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 28251 )
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);
void Initialize(HINSTANCE hInstance, PWSTR pCmdLine);
void MainLoop();
void Shutdown();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNUSED(hPrevInstance);
    UNUSED(nCmdShow);
    Initialize(hInstance, pCmdLine);
    MainLoop();
    Shutdown();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void Initialize(HINSTANCE hInstance, PWSTR pCmdLine) {
    UNUSED(hInstance);
    auto cmdString = StringUtils::ConvertUnicodeToMultiByte(std::wstring(pCmdLine ? pCmdLine : L""));
    g_theApp = new App(cmdString);
    g_theApp->Initialize();
}

void MainLoop() {
    while(!g_theApp->IsQuitting()) {
        g_theApp->RunFrame();
    }
}

void Shutdown() {
    delete g_theApp;
    g_theApp = nullptr;
}
