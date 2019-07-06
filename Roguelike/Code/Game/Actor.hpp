#pragma once

#include "Game/Entity.hpp"

class Actor : public Entity {
public:
    virtual ~Actor() = default;

    bool Acted() const;
    void Act(bool value);
    void Act();
    void DontAct();

    void Move(const IntVector2& direction);
    void MoveNorth();
    void MoveNorthEast();
    void MoveEast();
    void MoveSouthEast();
    void MoveSouth();
    void MoveSouthWest();
    void MoveWest();
    void MoveNorthWest();

protected:
private:
    bool CanMoveDiagonallyToNeighbor(const IntVector2& direction) const;
    bool _acted = false;
};
