#pragma once

#include "Engine/Core/Config.hpp"
#include "Engine/Core/EngineSubsystem.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include <memory>

namespace a2de {
    class JobSystem;
    class FileLogger;
    class Config;
    class Renderer;
    class Console;
    class InputSystem;
    class AudioSystem;
    class UISystem;
}

class Game;

class App : public a2de::EngineSubsystem {
public:
    explicit App(const std::string& cmdString);
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
    void RunMessagePump() const;

    void SetupEngineSystemPointers();
    void SetupEngineSystemChainOfResponsibility();

    virtual void BeginFrame() override;
    virtual void Update(a2de::TimeUtils::FPSeconds deltaSeconds) override;
    virtual void Render() const override;
    virtual void EndFrame() override;
    virtual bool ProcessSystemMessage(const EngineMessage& msg) noexcept override;

    void LogSystemDescription() const;

    bool _isQuitting = false;
    bool _current_focus = false;
    bool _previous_focus = false;

    std::unique_ptr<a2de::JobSystem> _theJobSystem{};
    std::unique_ptr<a2de::FileLogger> _theFileLogger{};
    std::unique_ptr<a2de::Config> _theConfig{};
    std::unique_ptr<a2de::Renderer> _theRenderer{};
    std::unique_ptr<a2de::Console> _theConsole{};
    std::unique_ptr<a2de::InputSystem> _theInputSystem{};
    std::unique_ptr<a2de::UISystem> _theUI{};
    std::unique_ptr<a2de::AudioSystem> _theAudioSystem{};
    std::unique_ptr<Game> _theGame{};

};
