#pragma once

#include "Game/Behavior.hpp"

class Pathfinder;

class PursueBehavior : public Behavior {
public:
    PursueBehavior() noexcept;
    explicit PursueBehavior(Actor* target) noexcept;

    void InitializePathfinding();

    virtual ~PursueBehavior() = default;

    void SetTarget(Actor* target) noexcept override;

    void Act(Actor* actor) noexcept override;
    float CalculateUtility() noexcept override;
protected:
private:

    Pathfinder* pather{};

    friend class ActorCommand;
    friend class MoveDownActorCommand;
    friend class MoveUpActorCommand;
    friend class MoveLeftActorCommand;
    friend class MoveRightActorCommand;
    friend class RestActorCommand;

};
