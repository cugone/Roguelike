#include "Game/MoveSouthEastCommand.hpp"

#include "Game/Actor.hpp"

void MoveSouthEastCommand::execute() {
    _actor->MoveSouthEast();
}
