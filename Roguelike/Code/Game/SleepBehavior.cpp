#include "Game/SleepBehavior.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Game/Actor.hpp"
#include "Game/Command.hpp"
#include "Game/RestCommand.hpp"

SleepBehavior::SleepBehavior() noexcept
    : Behavior()
{
    SetName("sleep");
}

void SleepBehavior::Act(Actor* actor) noexcept {
    auto command = RestCommand(actor);
    command.execute();
}

float SleepBehavior::CalculateUtility() noexcept {
    return 0.0f;
}
