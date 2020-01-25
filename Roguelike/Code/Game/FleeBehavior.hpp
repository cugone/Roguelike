#pragma once

#include "Game/Behavior.hpp"

class FleeBehavior : public Behavior {
public:
    FleeBehavior() noexcept;
    virtual ~FleeBehavior() = default;

    void Act(Actor* actor) noexcept override;
    float CalculateUtility() noexcept override;

protected:
private:

    friend class ActorCommand;
    friend class MoveDownActorCommand;
    friend class MoveUpActorCommand;
    friend class MoveLeftActorCommand;
    friend class MoveRightActorCommand;
    friend class RestActorCommand;

};
