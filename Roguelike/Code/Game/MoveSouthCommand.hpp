#pragma once

#include "Game/ActorCommand.hpp"

class MoveSouthCommand : public ActorCommand {
public:
    MoveSouthCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveSouthCommand() = default;
    virtual void execute() override;
protected:
private:
};
