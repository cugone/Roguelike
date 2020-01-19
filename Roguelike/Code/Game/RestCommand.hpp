#pragma once

#include "Game/ActorCommand.hpp"

class RestCommand : public ActorCommand {
public:
    explicit RestCommand(Actor* actor) : ActorCommand(actor) {};
    virtual ~RestCommand() = default;
    virtual void execute() override;
protected:
private:
};
