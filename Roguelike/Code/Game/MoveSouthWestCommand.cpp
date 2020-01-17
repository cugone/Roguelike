#include "Game/MoveSouthWestCommand.hpp"

#include "Game/Actor.hpp"

void MoveSouthWestCommand::execute() {
    _actor->MoveSouthWest();
}
