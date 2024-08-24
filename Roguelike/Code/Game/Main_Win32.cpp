#include "Engine/Core/EngineBase.hpp"
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
    UNUSED(pCmdLine);

    Engine<Game>::Initialize("RogueLike");
    Engine<Game>::Run();
    Engine<Game>::Shutdown();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
