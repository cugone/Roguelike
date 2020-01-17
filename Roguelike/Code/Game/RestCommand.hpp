#pragma once

#include "Game/ActorCommand.hpp"

class RestCommand : public ActorCommand {
public:
    RestCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~RestCommand() = default;
    virtual void execute() override;
protected:
private:
};
