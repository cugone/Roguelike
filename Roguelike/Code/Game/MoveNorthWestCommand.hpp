#pragma once

#include "Game/ActorCommand.hpp"

class MoveNorthWestCommand : public ActorCommand {
public:
    MoveNorthWestCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveNorthWestCommand() = default;
    virtual void execute() override;
protected:
private:
};
