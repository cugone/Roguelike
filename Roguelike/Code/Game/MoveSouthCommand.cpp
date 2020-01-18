#include "Game/MoveSouthCommand.hpp"

#include "Game/Actor.hpp"

void MoveSouthCommand::execute() {
    _actor->MoveSouth();
}
