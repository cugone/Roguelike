#include "Game/MoveNorthCommand.hpp"

#include "Game/Actor.hpp"

void MoveNorthCommand::execute() {
    _actor->MoveNorth();
}
