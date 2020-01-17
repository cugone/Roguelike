#include "Game/RestCommand.hpp"

#include "Game/Actor.hpp"

void RestCommand::execute() {
    _actor->Rest();
}
