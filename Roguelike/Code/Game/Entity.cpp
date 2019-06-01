#include "Game/Entity.hpp"

#include "Game/Map.hpp"

void Entity::Move(const IntVector2& direction) {
    if(CanMoveDiagonallyToNeighbor(direction)) {
        const auto target_position = position + direction;
        auto* target_tile = map->GetTile(target_position);
        if(target_tile && target_tile->IsPassable()) {
            position += direction;
            tile = target_tile;
        }
    }
}

bool Entity::CanMoveDiagonallyToNeighbor(const IntVector2& direction) {
    const auto target = position + direction;
    if(position.x == target.x || position.y == target.y) {
        return true;
    }
    if(auto* tile = map->GetTile(position.x, target.y)) {
        if(!tile->IsPassable()) {
            return false;
        }
    }
    if(auto* tile = map->GetTile(target.x, position.y)) {
        if(!tile->IsPassable()) {
            return false;
        }
    }
    return true;
}

void Entity::MoveWest() {
    Move(IntVector2{-1,0});
}

void Entity::MoveEast() {
    Move(IntVector2{1,0});
}

void Entity::MoveNorth() {
    Move(IntVector2{0,-1});
}

void Entity::MoveSouth() {
    Move(IntVector2{0,1});
}
