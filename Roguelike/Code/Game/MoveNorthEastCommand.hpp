#pragma once

#include "Game/ActorCommand.hpp"

class MoveNorthEastCommand : public ActorCommand {
public:
    explicit MoveNorthEastCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveNorthEastCommand() = default;
    virtual void execute() override;
protected:
private:
};
