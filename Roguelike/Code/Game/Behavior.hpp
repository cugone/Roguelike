#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <memory>
#include <string>

class Actor;

enum class BehaviorID {
    None
    ,Wander
    ,Flee
};

class Behavior {
public:
    virtual ~Behavior() = default;

    static std::string NameFromId(BehaviorID id);
    static BehaviorID IdFromName(std::string name);

    static std::shared_ptr<Behavior> Create(const XMLElement& element) noexcept;
    static std::shared_ptr<Behavior> Create(std::string name) noexcept;

    virtual void Act(Actor* actor) noexcept = 0;
    virtual float CalculateUtility() noexcept = 0;
    const std::string& GetName() const noexcept;

protected:
    Behavior() = default;
    explicit Behavior(Actor* target);
    void SetTarget(Actor* target) noexcept;
    Actor* GetTarget() const noexcept;

    void SetName(const std::string& name) noexcept;
private:
    Actor* _target{};
    std::string _name{"none"};
};
