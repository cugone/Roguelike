#pragma once

#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/FileLogger.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Console.hpp"
#include "Engine/Core/Config.hpp"
#include "Engine/UI/UISystem.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/App.hpp"
#include "Game/Game.hpp"

extern a2de::JobSystem* g_theJobSystem;
extern a2de::FileLogger* g_theFileLogger;
extern a2de::Renderer* g_theRenderer;
extern a2de::Console* g_theConsole;
extern a2de::Config* g_theConfig;
extern a2de::UISystem* g_theUISystem;
extern a2de::InputSystem* g_theInputSystem;
extern a2de::AudioSystem* g_theAudioSystem;
extern App* g_theApp;
extern Game* g_theGame;
extern a2de::EngineSubsystem* g_theSubsystemHead;
