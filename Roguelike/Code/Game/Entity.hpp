#pragma once

#include "Engine/Math/IntVector2.hpp"

class Map;
class Layer;
class Tile;

class Entity {
public:
    Entity() = default;
    Entity(const Entity& other) = default;
    Entity(Entity&& other) = default;
    Entity& operator=(const Entity& other) = default;
    Entity& operator=(Entity&& other) = default;
    virtual ~Entity() = default;

    void Move(const IntVector2& direction);
    void MoveWest();
    void MoveEast();
    void MoveNorth();
    void MoveSouth();

    Map* map = nullptr;
    Layer* layer = nullptr;
    Tile* tile = nullptr;

protected:
private:
    bool CanMoveDiagonallyToNeighbor(const IntVector2& direction);

    IntVector2 position{};
};
