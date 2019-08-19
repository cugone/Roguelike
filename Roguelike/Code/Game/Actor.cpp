#include "Game/Actor.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/Map.hpp"
#include "Game/Inventory.hpp"
#include "Game/Item.hpp"

std::multimap<std::string, std::unique_ptr<Actor>> Actor::s_registry{};

Actor* Actor::CreateActor(Map* map, const XMLElement& elem) {
    auto new_actor = std::make_unique<Actor>(map, elem);
    auto new_actor_name = new_actor->name;
    auto ptr = new_actor.get();
    s_registry.emplace(new_actor_name, std::move(new_actor));
    return ptr;
}

void Actor::ClearActorRegistry() noexcept {
    s_registry.clear();
}

Actor::Actor(Map* map, const XMLElement& elem) noexcept
    : Entity()
{
    this->map = map;
    this->layer = this->map->GetLayer(0);
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Actor failed to load.");
    }
}

Actor::Actor(Map* map, EntityDefinition* definition) noexcept
    : Entity(definition)
{
    this->map = map;
    this->layer = this->map->GetLayer(0);
    sprite = def->GetSprite();
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
        return Move(destination->GetCoords() - this->GetPosition());
    }
    return false;
}

bool Actor::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "actor", "", "name,lookAndFeel,position");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    const auto definitionName = DataUtils::ParseXmlAttribute(elem, "lookAndFeel", "");
    def = EntityDefinition::GetEntityDefinitionByName(definitionName);
    sprite = def->GetSprite();
    inventory = def->inventory;
    SetPosition(DataUtils::ParseXmlAttribute(elem, "position", IntVector2::ZERO));
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
