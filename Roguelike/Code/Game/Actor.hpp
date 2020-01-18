#pragma once

#include "Game/Behavior.hpp"
#include "Game/Entity.hpp"
#include "Game/Item.hpp"

#include <map>
#include <memory>
#include <vector>

class Behavior;

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

    void Rest();
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

    Item* IsEquipped(const EquipSlot& slot);
    void Equip(const EquipSlot& slot, Item* item);
    void Unequip(const EquipSlot& slot);

    const std::vector<Item*>& GetEquipment() const noexcept;

    virtual void SetPosition(const IntVector2& position) override;

    float visibility = 2.0f;

    void SetBehavior(const std::string& behaviorName);
    Behavior* GetCurrentBehavior() const noexcept;

protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    bool CanMoveDiagonallyToNeighbor(const IntVector2& direction) const;

    std::vector<Item*> GetAllEquipmentOfType(const EquipSlot& slot) const;
    std::vector<Item*> GetAllHairEquipment() const;
    std::vector<Item*> GetAllHeadEquipment() const;
    std::vector<Item*> GetAllBodyEquipment() const;
    std::vector<Item*> GetAllLeftArmEquipment() const;
    std::vector<Item*> GetAllRightArmEquipment() const;
    std::vector<Item*> GetAllLegsEquipment() const;
    std::vector<Item*> GetAllFeetEquipment() const;

    static std::multimap<std::string, std::unique_ptr<Actor>> s_registry;
    std::vector<Item*> _equipment = std::vector<Item*>(static_cast<std::size_t>(EquipSlot::Max));
    Behavior* _active_behavior{};
    std::map<std::string, std::unique_ptr<class Behavior>> _available_behaviors{};
    bool _acted = false;
};
