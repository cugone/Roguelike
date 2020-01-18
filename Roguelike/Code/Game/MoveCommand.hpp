#pragma once

#include "Engine/Math/IntVector2.hpp"

#include "Game/ActorCommand.hpp"

class MoveCommand : public ActorCommand {
public:
    MoveCommand(Actor* actor, const IntVector2& direction) : ActorCommand(actor), _direction(direction) {};
    virtual ~MoveCommand() = default;
    virtual void execute() override;
protected:
private:
    IntVector2 _direction;
};
