#pragma once

#include "Game/ActorCommand.hpp"

class MoveSouthEastCommand : public ActorCommand {
public:
    MoveSouthEastCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveSouthEastCommand() = default;
    virtual void execute() override;
protected:
private:
};
