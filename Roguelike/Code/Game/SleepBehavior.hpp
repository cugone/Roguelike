#pragma once

#include "Game/Behavior.hpp"

class SleepBehavior : public Behavior {
public:
    SleepBehavior() noexcept;
    virtual ~SleepBehavior() = default;

    void Act(Actor* actor) noexcept override;
    float CalculateUtility() noexcept override;

protected:
private:

    friend class RestActorCommand;

};
