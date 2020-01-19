#pragma once

#include "Game/ActorCommand.hpp"

class MoveEastCommand : public ActorCommand {
public:
    explicit MoveEastCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveEastCommand() = default;
    virtual void execute() override;
protected:
private:
};
