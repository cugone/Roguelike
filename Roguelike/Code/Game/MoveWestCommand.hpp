#pragma once

#include "Game/ActorCommand.hpp"

class MoveWestCommand : public ActorCommand {
public:
    explicit MoveWestCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveWestCommand() = default;
    virtual void execute() override;
protected:
private:
};
