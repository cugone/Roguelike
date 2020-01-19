#pragma once

#include "Game/ActorCommand.hpp"

class MoveNorthWestCommand : public ActorCommand {
public:
    explicit MoveNorthWestCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveNorthWestCommand() = default;
    virtual void execute() override;
protected:
private:
};
