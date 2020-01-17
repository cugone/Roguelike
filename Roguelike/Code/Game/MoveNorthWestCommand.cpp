#include "Game/MoveNorthWestCommand.hpp"

#include "Game/Actor.hpp"

void MoveNorthWestCommand::execute() {
    _actor->MoveNorthWest();
}
