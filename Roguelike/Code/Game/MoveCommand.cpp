#include "Game/MoveCommand.hpp"

#include "Game/Actor.hpp"

void MoveCommand::execute() {
    _actor->Move(_direction);
}
