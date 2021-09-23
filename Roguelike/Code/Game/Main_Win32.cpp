#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Platform/Win.hpp"
#include "Game/Game.hpp"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 28251 )
#endif

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNUSED(hInstance);
    UNUSED(hPrevInstance);
    UNUSED(nCmdShow);
    const auto cmdString = StringUtils::ConvertUnicodeToMultiByte(std::wstring(pCmdLine ? pCmdLine : L""));
    Engine<Game>::Initialize("RogueLike", cmdString);
    Engine<Game>::Run();
    Engine<Game>::Shutdown();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
