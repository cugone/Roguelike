#include "Game/MoveWestCommand.hpp"

#include "Game/Actor.hpp"

void MoveWestCommand::execute() {
    _actor->MoveWest();
}
