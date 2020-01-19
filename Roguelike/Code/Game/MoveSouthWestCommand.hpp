#pragma once

#include "Game/ActorCommand.hpp"

class MoveSouthWestCommand : public ActorCommand {
public:
    explicit MoveSouthWestCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~MoveSouthWestCommand() = default;
    virtual void execute() override;
protected:
private:
};
