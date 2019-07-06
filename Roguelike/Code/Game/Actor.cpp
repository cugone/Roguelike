#include "Game/Actor.hpp"

#include "Game/Map.hpp"

bool Actor::Acted() const {
    return _acted;
}

void Actor::Act(bool value) {
    _acted = value;
}

void Actor::Act() {
    Act(true);
}

void Actor::DontAct() {
    Act(false);
}

bool Actor::CanMoveDiagonallyToNeighbor(const IntVector2& direction) const {
    const auto& pos = GetPosition();
    const auto target = pos + direction;
    if(pos.x == target.x || pos.y == target.y) {
        return true;
    }
    {
        const auto test_tiles = map->GetTiles(pos.x, target.y);
        for(const auto* t : test_tiles) {
            if(t->IsSolid()) {
                return false;
            }
        }
    }
    {
        const auto test_tiles = map->GetTiles(target.x, pos.y);
        for(const auto* t : test_tiles) {
            if(t->IsSolid()) {
                return false;
            }
        }
    }
    return true;
}

void Actor::Move(const IntVector2& direction) {
    if(CanMoveDiagonallyToNeighbor(direction)) {
        const auto pos = GetPosition();
        const auto target_position = pos + direction;
        auto target_tile = map->GetTile(target_position.x, target_position.y, 0);
        if(target_tile) {
            if(target_tile->IsPassable()) {
                SetPosition(target_position);
            } else {
                auto target_entity = target_tile->entity;
                if(target_entity) {
                    Fight(*this, *target_entity);
                }
            }
        }
    }
    Act();
}

void Actor::MoveNorth() {
    Move(IntVector2{ 0,-1 });
}

void Actor::MoveNorthEast() {
    Move(IntVector2{ 1,-1 });
}

void Actor::MoveEast() {
    Move(IntVector2{ 1,0 });
}

void Actor::MoveSouthEast() {
    Move(IntVector2{ 1,1 });
}

void Actor::MoveSouth() {
    Move(IntVector2{ 0,1 });
}

void Actor::MoveSouthWest() {
    Move(IntVector2{ -1,1 });
}

void Actor::MoveWest() {
    Move(IntVector2{ -1,0 });
}

void Actor::MoveNorthWest() {
    Move(IntVector2{ -1,-1 });
}
