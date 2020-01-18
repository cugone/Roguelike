#include "Game/Actor.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/Behavior.hpp"
#include "Game/Inventory.hpp"
#include "Game/Item.hpp"

#include <algorithm>

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

void Actor::Rest() {
    Act();
}

bool Actor::MoveTo(Tile* destination) {
    if(destination) {
        return Move(destination->GetCoords() - this->GetPosition());
    }
    return false;
}

bool Actor::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "actor", "", "name,lookAndFeel,position", "behaviors");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    const auto definitionName = DataUtils::ParseXmlAttribute(elem, "lookAndFeel", "");
    def = EntityDefinition::GetEntityDefinitionByName(definitionName);
    sprite = def->GetSprite();
    inventory = def->inventory;
    _equipment = def->equipment;
    for(const auto& e : _equipment) {
        if (e) {
            AdjustStatModifiers(e->GetStatModifiers());
        }
    }
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

std::vector<Item*> Actor::GetAllEquipmentOfType(const EquipSlot& slot) const {
    decltype(_equipment) result{};
    auto pred = [slot](const Item* a) { return a->GetEquipSlot() == slot; };
    auto count = static_cast<std::size_t>(std::count_if(std::begin(inventory), std::end(inventory), pred));
    result.reserve(count);
    std::copy_if(std::begin(inventory), std::end(inventory), std::back_inserter(result), pred);
    result.shrink_to_fit();
    return result;
}

std::vector<Item*> Actor::GetAllHairEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::Hair);
}

std::vector<Item*> Actor::GetAllHeadEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::Head);
}

std::vector<Item*> Actor::GetAllBodyEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::Body);
}

std::vector<Item*> Actor::GetAllLeftArmEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::LeftArm);
}

std::vector<Item*> Actor::GetAllRightArmEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::RightArm);
}

std::vector<Item*> Actor::GetAllLegsEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::Legs);
}

std::vector<Item*> Actor::GetAllFeetEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::Feet);
}

bool Actor::Move(const IntVector2& direction) {
    bool moved = false;
    if(CanMoveDiagonallyToNeighbor(direction)) {
        const auto pos = GetPosition();
        const auto target_position = pos + direction;
        const auto test_tiles = map->GetTiles(target_position);
        if(test_tiles.empty()) {
            return false;
        }
        for(const auto* t : test_tiles) {
            if(!t->IsPassable()) {
                return false;
            }
        }
        SetPosition(target_position);
        moved = true;
        OnMove.Trigger(pos, target_position);
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

Item* Actor::IsEquipped(const EquipSlot& slot) {
    return _equipment[static_cast<std::size_t>(slot)];
}

void Actor::Equip(const EquipSlot& slot, Item* item) {
    auto& current_equipment = _equipment[static_cast<std::size_t>(slot)];
    if(current_equipment) {
        this->AdjustStatModifiers(-current_equipment->GetStatModifiers());
    }
    current_equipment = item;
    if(current_equipment) {
        this->AdjustStatModifiers(current_equipment->GetStatModifiers());
    }
}

void Actor::Unequip(const EquipSlot& slot) {
    Equip(slot, nullptr);
}

const std::vector<Item*>& Actor::GetEquipment() const noexcept {
    return _equipment;
}

void Actor::SetPosition(const IntVector2& position) {
    auto cur_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    cur_tile->actor = nullptr;
    Entity::SetPosition(position);
    auto next_tile = map->GetTile(_position.x, _position.y, layer->z_index);
    next_tile->actor = this;
    tile = next_tile;
}

void Actor::SetBehavior(const std::string& behaviorName) {
    const auto found_iter = _available_behaviors.find(behaviorName);
    const auto is_available = found_iter != std::end(_available_behaviors);
    if(is_available) {
        _active_behavior = found_iter->second.get();
    }
}

Behavior* Actor::GetCurrentBehavior() const noexcept {
    return _active_behavior;
}
