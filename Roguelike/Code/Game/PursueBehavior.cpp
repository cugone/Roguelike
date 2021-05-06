#include "Game/PursueBehavior.hpp"

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

#include "Game/Map.hpp"

#include "Game/Pathfinder.hpp"

PursueBehavior::PursueBehavior() noexcept
    : PursueBehavior(nullptr)
{}

PursueBehavior::PursueBehavior(Actor* target) noexcept
    : Behavior(target)
{
    SetName("pursue");
    SetTarget(target);
}

void PursueBehavior::InitializePathfinding() {
    auto* target = GetTarget();
    if(target) {
        pather = target->map->GetPathfinder();
        const auto dims = a2de::IntVector2{target->map->CalcMaxDimensions()};
        const auto w = dims.x;
        const auto h = dims.y;
        pather->Initialize(w, h);
    }
}

void PursueBehavior::SetTarget(Actor* target) noexcept {
    Behavior::SetTarget(target);
    InitializePathfinding();
}

void PursueBehavior::Act(Actor* actor) noexcept {
    const auto viable = [this, actor](const a2de::IntVector2& a)->bool {
        const auto coords = a2de::IntVector3{a, 0};
        const auto* map = actor->map;
        return map->IsTilePassable(coords);
    };
    const auto h = [](const a2de::IntVector2& a, const a2de::IntVector2& b) {
        return a2de::MathUtils::CalculateManhattanDistance(a, b);
    };
    const auto d = [this](const a2de::IntVector2& a, const a2de::IntVector2& b) {
        const auto va = a2de::Vector2{a} + a2de::Vector2{0.5f, 0.5f};
        const auto vb = a2de::Vector2{b} + a2de::Vector2{0.5f, 0.5f};
        return a2de::MathUtils::CalcDistance(va, vb);
    };
    const auto& my_loc = actor->GetPosition();
    const auto& target_loc = GetTarget()->GetPosition();
    pather->AStar(my_loc, target_loc, viable, h, d);
    const auto path = pather->GetResult();
    for(auto& node : path) {
        const auto coords = a2de::IntVector3{node->coords, 0};
        const auto* map = actor->map;
        auto* tile = map->GetTile(coords);
        if(tile) {
            tile->color = a2de::Rgba::White;
        }
    }

}

float PursueBehavior::CalculateUtility() noexcept {
    return 0.0f;
}
