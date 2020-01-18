#pragma once

#include "Game/ActorCommand.hpp"

class MoveNorthCommand : public ActorCommand {
public:
    MoveNorthCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveNorthCommand() = default;
    virtual void execute() override;
protected:
private:
};
