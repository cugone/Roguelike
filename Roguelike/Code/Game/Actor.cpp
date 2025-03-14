#include "Game/Actor.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/Game.hpp"
#include "Game/Behavior.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Inventory.hpp"
#include "Game/Item.hpp"
#include "Game/Map.hpp"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <numeric>
#include <sstream>

std::multimap<std::string, std::unique_ptr<Actor>> Actor::s_registry{};

Actor* Actor::CreateActor(Map* map, const XMLElement& elem) {
    auto new_actor = std::make_unique<Actor>(map, elem);
    std::string new_actor_name = new_actor->name;
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
    GUARANTEE_OR_DIE(LoadFromXml(elem), "Actor failed to load.");
    OnDamage.Subscribe_method(this, &Actor::ApplyDamage);
    OnFight.Subscribe_method(this, &Actor::ResolveAttack);
    OnMiss.Subscribe_method(this, &Actor::AttackerMissed);
}

Actor::Actor(Map* map, EntityDefinition* definition) noexcept
    : Entity(definition)
{
    this->map = map;
    this->layer = this->map->GetLayer(0);
    sprite = definition->GetSprite();
    OnDamage.Subscribe_method(this, &Actor::ApplyDamage);
    OnFight.Subscribe_method(this, &Actor::ResolveAttack);
    OnMiss.Subscribe_method(this, &Actor::AttackerMissed);
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
    DataUtils::ValidateXmlElement(elem, "actor", "", "name,lookAndFeel", "", "position,behavior");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    const auto definitionName = DataUtils::ParseXmlAttribute(elem, "lookAndFeel", std::string{});
    auto* def = EntityDefinition::GetEntityDefinitionByName(definitionName);
    sprite = def->GetSprite();
    inventory = def->inventory;
    const auto behaviorName = DataUtils::ParseXmlAttribute(elem, "behavior", std::string{"none"});
    this->SetBehavior(Behavior::IdFromName(behaviorName));
    _equipment = def->equipment;
    for(const auto& e : _equipment) {
        if(e) {
            AdjustStatModifiers(e->GetStatModifiers());
        }
    }
    if(DataUtils::HasAttribute(elem, "position")) {
        SetPosition(DataUtils::ParseXmlAttribute(elem, "position", IntVector2::Zero));
    }
    return true;
}

bool Actor::CanMoveDiagonallyToNeighbor(const IntVector2& direction) const {
    const auto& pos = GetPosition();
    const auto target = pos + direction;
    if(pos.x == target.x || pos.y == target.y) {
        return true;
    }
    if(const auto test_tiles = map->GetTiles(pos.x, target.y); test_tiles.has_value()) {
        for(const auto* t : (*test_tiles)) {
            if(t && t->IsSolid()) {
                return false;
            }
        }
    }
    if(const auto test_tiles = map->GetTiles(target.x, pos.y); test_tiles.has_value()) {
        for(const auto* t : *test_tiles) {
            if(t && t->IsSolid()) {
                return false;
            }
        }
    }
    return true;
}

void Actor::ResolveAttack(Entity& attacker, Entity& defender) {
    if(attacker.GetFaction() == defender.GetFaction()) {
        return;
    }
    auto aStats = attacker.GetStats();
    auto dStats = defender.GetStats();
    const auto aAtt = aStats.GetStat(StatsID::Attack);
    const auto aSpd = aStats.GetStat(StatsID::Speed);
    const auto dDef = dStats.GetStat(StatsID::Defense);
    const auto dEva = dStats.GetStat(StatsID::Evasion);
    const auto aLck = aStats.GetStat(StatsID::Luck);
    const auto aLvl = aStats.GetStat(StatsID::Level);
    const auto dLvl = dStats.GetStat(StatsID::Level);
    const auto damageType = DamageType::Physical;
    switch(damageType) {
    case DamageType::None:
        break;
    case DamageType::Physical:
    {
        if(aSpd < dEva) {
            defender.OnMiss.Trigger();
            return;
        }
        const auto [result, crit] = [&]() {
            auto result = aAtt - dDef;
            auto crit = false;
            if (aAtt < dDef) {
                result = 0L;
            }
            else {
                const auto chance = (std::floor((aLck + aLvl - dLvl) / 4.0f)) / 100.0f;
                crit = MathUtils::IsPercentChance(chance);
                if (crit) {
                    result *= 2;
                }
            }
            return std::tie(result, crit);
        }(); //IIIL
        defender.OnDamage.Trigger(damageType, result, crit);
        break;
    }
    default:
        break;
    }
}

void Actor::ApplyDamage(DamageType type, long amount, bool crit) {
    switch(type) {
    case DamageType::None:
        break;
    case DamageType::Physical:
    {
        auto my_total_stats = GetStats();
        if(const auto new_health = my_total_stats.AdjustStat(StatsID::Health, -amount); new_health <= 0L) {
            OnDestroy.Trigger();
            map->KillEntity(*this);
        }
        break;
    }
    default:
        break;
    }
    //Make damage text
    TextEntityDesc desc{};
    desc.font = GetGameAs<Game>()->ingamefont;
    desc.color = amount < 0 ? Rgba::Green : (crit ? Rgba::Yellow : Rgba::White);
    const auto abs_damage = std::abs(amount);
    desc.text = std::vformat("{}", std::make_format_args(abs_damage));
    map->CreateTextEntityAt(tile->GetCoords(), desc);
}

void Actor::AttackerMissed() {
    TextEntityDesc desc{};
    desc.font = GetGameAs<Game>()->ingamefont;
    desc.color = Rgba::White;
    desc.text = "MISS";
    map->CreateTextEntityAt(tile->GetCoords(), desc);
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

std::vector<Item*> Actor::GetAllCapeEquipment() const {
    return GetAllEquipmentOfType(EquipSlot::Cape);
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
        const auto& pos = GetPosition();
        const auto target_position = pos + direction;
        if(const auto test_tiles = map->GetTiles(target_position); test_tiles.has_value()) {
            for(const auto* t : *test_tiles) {
                if(!t || !t->IsPassable()) {
                    return false;
                }
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
    return Move(IntVector2{0,-1});
}

bool Actor::MoveNorthEast() {
    return Move(IntVector2{1,-1});
}

bool Actor::MoveEast() {
    return Move(IntVector2{1,0});
}

bool Actor::MoveSouthEast() {
    return Move(IntVector2{1,1});
}

bool Actor::MoveSouth() {
    return Move(IntVector2{0,1});
}

bool Actor::MoveSouthWest() {
    return Move(IntVector2{-1,1});
}

bool Actor::MoveWest() {
    return Move(IntVector2{-1,0});
}

bool Actor::MoveNorthWest() {
    return Move(IntVector2{-1,-1});
}

Item* Actor::IsEquipped(const EquipSlot& slot) const noexcept {
    if(slot != EquipSlot::None) {
        return _equipment[static_cast<std::size_t>(slot)];
    }
    return nullptr;
}

bool Actor::IsEquipped(const EquipSlot& slot, const std::string& itemName) const noexcept {
    if(const auto* equipped_item = IsEquipped(slot); equipped_item != nullptr) {
        return equipped_item->GetName() == itemName;
    }
    return false;
}

void Actor::Equip(const EquipSlot& slot, Item* item) {
    if(slot == EquipSlot::None) {
        return;
    }
    auto& current_equipment = _equipment[static_cast<std::size_t>(slot)];
    if(current_equipment) { //Remove existing equipment, reduce your stats by equipment's stats.
        this->AdjustStatModifiers(-current_equipment->GetStatModifiers());
    }
    current_equipment = item;
    if(current_equipment) { //Don new equipment, add equipment's stats to your stats.
        this->AdjustStatModifiers(current_equipment->GetStatModifiers());
    }
    CalculateLightValue();
}

void Actor::Unequip(const EquipSlot& slot) {
    Equip(slot, nullptr);
}

const std::vector<Item*>& Actor::GetEquipment() const noexcept {
    return _equipment;
}

void Actor::SetPosition(const IntVector2& position) {
    if(auto* cur_tile = map->GetTile(_position.x, _position.y, layer->z_index)) {
        cur_tile->actor = nullptr;
        Entity::SetPosition(position);
        if(auto* next_tile = map->GetTile(_position.x, _position.y, layer->z_index)) {
            next_tile->actor = this;
            tile = next_tile;
            if(tile->HasInventory()) {
                Inventory::TransferAll(*tile->inventory, inventory);
            }
        }
    }
}

void Actor::SetBehavior(BehaviorID id) {
    const auto behaviorName = Behavior::NameFromId(id);
    if(auto* def = EntityDefinition::GetEntityDefinitionByName(name)) {
        const auto& behaviors = def->GetAvailableBehaviors();
        const auto found_iter = std::find_if(std::begin(behaviors), std::end(behaviors), [this, &behaviorName](auto b) { return b->GetName() == behaviorName; });
        const auto is_available = found_iter != std::end(behaviors);
        if(is_available) {
            _active_behavior = found_iter->get();
            _active_behavior->SetTarget(this->map->player);
        }
    }
}

Behavior* Actor::GetCurrentBehavior() const noexcept {
    return _active_behavior;
}

void Actor::CalculateLightValue() noexcept {
    const auto acc_op = [](const uint32_t a, const Item* b) {
        const auto b_value = b ? b->GetLightValue() : min_light_value;
        return a + b_value;
    };
    const auto value = _self_illumination + std::accumulate(std::cbegin(_equipment), std::cend(_equipment), uint32_t{0u}, acc_op);
    if(value != GetLightValue()) {
        SetLightValue(value);
        tile->DirtyLight();
        for(auto* neighbor : tile->GetCardinalNeighbors()) {
            if(neighbor) {
                neighbor->DirtyLight();
            }
        }
    }
}
