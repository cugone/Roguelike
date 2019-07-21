#include "Game/Actor.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/Map.hpp"

std::map<std::string, std::unique_ptr<Actor>> Actor::s_registry{};

void Actor::CreateActor(const XMLElement& elem) {
    auto new_actor = std::make_unique<Actor>(elem);
    s_registry.try_emplace(new_actor->name, std::move(new_actor));
}

Actor::Actor(const XMLElement& elem) noexcept
    : Entity(elem)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Actor failed to load.");
    }
    s_registry.try_emplace(name, std::make_unique<Actor>(*this));
}

Actor::Actor(EntityDefinition* definition) noexcept
    : Entity(definition)
{
    s_registry.try_emplace(name, std::make_unique<Actor>(*this));
}

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

bool Actor::MoveTo(Tile* destination) {
    if(destination) {
        return Move(destination->GetCoords());
    }
    return false;
}

bool Actor::LoadFromXml(const XMLElement& /*elem*/) {
    //TODO: Start Here
    return true;
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

bool Actor::Move(const IntVector2& direction) {
    bool moved = false;
    if(CanMoveDiagonallyToNeighbor(direction)) {
        const auto pos = GetPosition();
        const auto target_position = pos + direction;
        auto target_tile = map->GetTile(target_position.x, target_position.y, 0);
        if(target_tile) {
            if(target_tile->IsPassable()) {
                SetPosition(target_position);
                moved = true;
            }
        }
    }
    Act();
    return moved;
}

bool Actor::MoveNorth() {
    return Move(IntVector2{ 0,-1 });
}

bool Actor::MoveNorthEast() {
    return Move(IntVector2{ 1,-1 });
}

bool Actor::MoveEast() {
    return Move(IntVector2{ 1,0 });
}

bool Actor::MoveSouthEast() {
    return Move(IntVector2{ 1,1 });
}

bool Actor::MoveSouth() {
    return Move(IntVector2{ 0,1 });
}

bool Actor::MoveSouthWest() {
    return Move(IntVector2{ -1,1 });
}

bool Actor::MoveWest() {
    return Move(IntVector2{ -1,0 });
}

bool Actor::MoveNorthWest() {
    return Move(IntVector2{ -1,-1 });
}
