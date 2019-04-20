#pragma once

#include "Engine/Core/Config.hpp"
#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include <memory>

class JobSystem;
class FileLogger;
class Config;
class Renderer;
class Game;
class Console;
class InputSystem;
class AudioSystem;
class UISystem;

class App : public EngineSubsystem {
public:
    App(const std::string& cmdString);
    App(const App& other) = default;
    App(App&& other) = default;
    App& operator=(const App& other) = default;
    App& operator=(App&& other) = default;
    virtual ~App();

    virtual void Initialize() override;
    void RunFrame();

    bool IsQuitting() const;
    void SetIsQuitting(bool value);

    bool HasFocus() const;
    bool LostFocus() const;
    bool GainedFocus() const;

protected:
private:
    virtual void BeginFrame() override;
    virtual void Update(TimeUtils::FPSeconds deltaSeconds) override;
    virtual void Render() const override;
    virtual void EndFrame() override;
    virtual bool ProcessSystemMessage(const EngineMessage& msg) override;

    void LogSystemDescription() const;
    bool ShowQuitRequest() const;

    bool _isQuitting = false;
    bool _current_focus = false;
    bool _previous_focus = false;

    std::unique_ptr<JobSystem> _theJobSystem{};
    std::unique_ptr<FileLogger> _theFileLogger{};
    std::unique_ptr<Config> _theConfig{};
    std::unique_ptr<Renderer> _theRenderer{};
    std::unique_ptr<UISystem> _theUI{};
    std::unique_ptr<Console> _theConsole{};
    std::unique_ptr<InputSystem> _theInputSystem{};
    std::unique_ptr<AudioSystem> _theAudioSystem{};
    std::unique_ptr<Game> _theGame{};

};
