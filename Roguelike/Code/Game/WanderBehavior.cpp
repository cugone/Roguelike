#include "Game/WanderBehavior.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Game/Actor.hpp"
#include "Game/Command.hpp"
#include "Game/RestCommand.hpp"
#include "Game/MoveCommand.hpp"
#include "Game/MoveNorthCommand.hpp"
#include "Game/MoveSouthCommand.hpp"
#include "Game/MoveEastCommand.hpp"
#include "Game/MoveWestCommand.hpp"
#include "Game/MoveNorthEastCommand.hpp"
#include "Game/MoveNorthWestCommand.hpp"
#include "Game/MoveSouthEastCommand.hpp"
#include "Game/MoveSouthWestCommand.hpp"

WanderBehavior::WanderBehavior() noexcept
    : Behavior()
{
    SetName("wander");
}

void WanderBehavior::Act(Actor* actor) noexcept {
    const auto direction = a2de::MathUtils::GetRandomIntLessThan(9);
    switch(direction) {
    case 0:
    {
        auto command = RestCommand(actor);
        command.execute();
        break;
    }
    case 1:
    {
        auto command = MoveNorthWestCommand(actor);
        command.execute();
        break;
    }
    case 2:
    {
        auto command = MoveNorthCommand(actor);
        command.execute();
        break;
    }
    case 3:
    {
        auto command = MoveNorthEastCommand(actor);
        command.execute();
        break;
    }
    case 4:
    {
        auto command = MoveEastCommand(actor);
        command.execute();
        break;
    }
    case 5:
    {
        auto command = MoveSouthEastCommand(actor);
        command.execute();
        break;
    }
    case 6:
    {
        auto command = MoveSouthCommand(actor);
        command.execute();
        break;
    }
    case 7:
    {
        auto command = MoveSouthWestCommand(actor);
        command.execute();
        break;
    }
    case 8:
    {
        auto command = MoveWestCommand(actor);
        command.execute();
        break;
    }
    }
}

float WanderBehavior::CalculateUtility() noexcept {
    return 0.0f;
}
