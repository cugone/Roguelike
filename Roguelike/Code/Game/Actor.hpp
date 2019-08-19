#pragma once

#include "Game/Entity.hpp"

#include <map>
#include <memory>

class Actor : public Entity {
public:

    static Actor* CreateActor(Map* map, const XMLElement& elem);
    static void ClearActorRegistry() noexcept;

    Actor() = default;
    Actor(const Actor& other) = default;
    Actor(Actor&& other) = default;
    Actor& operator=(const Actor& rhs) = default;
    Actor& operator=(Actor&& rrhs) = default;
    virtual ~Actor() = default;

    Actor(Map* map, EntityDefinition* definition) noexcept;
    Actor(Map* map, const XMLElement& elem) noexcept;

    bool Acted() const;
    void Act(bool value);
    void Act();
    void DontAct();

    bool MoveTo(Tile* destination);
    bool Move(const IntVector2& direction);
    bool MoveNorth();
    bool MoveNorthEast();
    bool MoveEast();
    bool MoveSouthEast();
    bool MoveSouth();
    bool MoveSouthWest();
    bool MoveWest();
    bool MoveNorthWest();

protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    bool CanMoveDiagonallyToNeighbor(const IntVector2& direction) const;

    static std::multimap<std::string, std::unique_ptr<Actor>> s_registry;
    bool _acted = false;
};
