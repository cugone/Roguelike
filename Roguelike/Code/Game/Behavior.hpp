#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <memory>
#include <string>

class Actor;

class Behavior {
public:
    virtual ~Behavior() = default;

    static std::shared_ptr<Behavior> Create(const XMLElement& element) noexcept;
    static std::shared_ptr<Behavior> Create(std::string name) noexcept;

    virtual void Act(Actor* actor) noexcept = 0;
    virtual float CalculateUtility() noexcept = 0;
protected:
    Behavior() = default;
    explicit Behavior(Actor* target);
    void SetTarget(Actor* target) noexcept;
    Actor* GetTarget() const noexcept;
private:
    Actor* _target{};
};
