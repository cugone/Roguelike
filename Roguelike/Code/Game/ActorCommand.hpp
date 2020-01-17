#pragma once

#include "Game/Command.hpp"

class Actor;

class ActorCommand : public Command {
public:
    ActorCommand() = delete;
    ActorCommand(Actor* actor) : Command(), _actor(actor) {};
    virtual ~ActorCommand() = default;
    virtual void execute() = 0;
protected:
    Actor* _actor{};
private:
};
