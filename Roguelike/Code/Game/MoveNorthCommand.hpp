#pragma once

#include "Game/ActorCommand.hpp"

class MoveNorthCommand : public ActorCommand {
public:
    explicit MoveNorthCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveNorthCommand() = default;
    virtual void execute() override;
protected:
private:
};
