#include "Engine/Core/EngineBase.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ThreadUtils.hpp"
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
    UNUSED(pCmdLine);
    PROFILE_BENCHMARK_BEGIN("RogueLike", "Data/Benchmarks/benchmark.json");
    ProfileMetadata metadata{};
    metadata.threadName = "Main";
    metadata.threadID = std::this_thread::get_id();
    metadata.processName = "RogueLike";
    metadata.processSortIndex = 0;
    metadata.threadSortIndex = 0;
    metadata.ProcessID = ThreadUtils::GetProcessIDFromThisThread();

    Instrumentor::Get().WriteSessionData(MetaDataCategory::ProcessName, metadata);
    Instrumentor::Get().WriteSessionData(MetaDataCategory::ProcessSortIndex, metadata);
    Instrumentor::Get().WriteSessionData(MetaDataCategory::ThreadName, metadata);
    Instrumentor::Get().WriteSessionData(MetaDataCategory::ThreadSortIndex, metadata);
    Engine<Game>::Initialize("RogueLike");
    Engine<Game>::Run();
    Engine<Game>::Shutdown();
    PROFILE_BENCHMARK_END();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
