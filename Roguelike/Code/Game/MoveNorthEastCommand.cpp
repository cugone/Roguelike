#include "Game/MoveNorthEastCommand.hpp"

#include "Game/Actor.hpp"

void MoveNorthEastCommand::execute() {
    _actor->MoveNorthEast();
}
