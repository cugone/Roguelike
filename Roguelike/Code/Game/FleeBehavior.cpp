#include "Game/FleeBehavior.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
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

#include <algorithm>

FleeBehavior::FleeBehavior() noexcept
    : Behavior()
{
    SetName("flee");
}

void FleeBehavior::Act(Actor* actor) noexcept {
    const auto* player = actor->map->player;
    const auto* player_tile = player->tile;
    int x = -1;
    int y = -1;
    unsigned int max_distance = 0;
    auto my_tile = actor->tile;
    Tile* target_tile = my_tile;
    for(; y < 2; ++y) {
        for(; x < 2; ++x) {
            if(const auto cur_tile = my_tile->GetNeighbor(a2de::IntVector3(x, y, 0))) {
                const auto calc_distance = a2de::MathUtils::CalculateManhattanDistance(cur_tile->GetCoords(), player_tile->GetCoords());
                if(max_distance < calc_distance) {
                    max_distance = calc_distance;
                    target_tile = cur_tile;
                }
            }
        }
    }
    actor->MoveTo(target_tile);
}

float FleeBehavior::CalculateUtility() noexcept {
    return 0.0f;
}
