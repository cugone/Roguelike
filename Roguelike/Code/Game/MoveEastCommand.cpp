#include "Game/MoveEastCommand.hpp"

#include "Game/Actor.hpp"

void MoveEastCommand::execute() {
    _actor->MoveEast();
}
