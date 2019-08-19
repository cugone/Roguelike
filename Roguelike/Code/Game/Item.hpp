#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Game/Inventory.hpp"
#include "Game/Stats.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class AnimatedSprite;
class Inventory;
class ItemBuilder;

enum class EquipSlot {
    None
    , Hair
    , Head
    , Body
    , LeftArm
    , RightArm
    , Legs
    , Feet
    , Max
};

EquipSlot EquipSlotFromString(std::string str);
std::string EquipSlotToString(const EquipSlot& slot);

class Item {
public:

    static Item* CreateItem(ItemBuilder& builder);
    static void ClearItemRegistry();

    static std::map<std::string, std::unique_ptr<Item>> s_registry;

    Item(const Item& other) = default;
    Item(Item&& other) = default;
    Item& operator=(const Item& other) = default;
    Item& operator=(Item&& other) = default;
    ~Item() = default;

    explicit Item(ItemBuilder& builder) noexcept;

    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;

    const Inventory& GetInventory() const noexcept;
    Inventory& GetInventory() noexcept;

    const Stats GetStatModifiers() const;
    Stats GetStatModifiers();

    static Item* GetItem(const std::string& name);

    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();

    std::string GetName() const noexcept;

    bool HasOwningInventory() const noexcept;
    bool IsChildInventory() const noexcept;
    const Inventory* OwningInventory() const noexcept;
    Inventory* OwningInventory() noexcept;

    std::size_t GetCount() const noexcept;
    void IncrementCount() noexcept;
    void DecrementCount() noexcept;
    void SetCount(std::size_t newCount) noexcept;

protected:
private:
    std::string _name{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    Inventory* _parent_inventory{};
    Inventory _my_inventory{};
    Stats _stat_modifiers{};
    EquipSlot _slot{};
    std::size_t _stack_size = 0;
    std::size_t _max_stack_size = 1;
}; //End Item

class ItemBuilder {
public:
    Item* Build() noexcept;

    ItemBuilder() = default;
    ItemBuilder(const ItemBuilder& other) noexcept = default;
    ItemBuilder(ItemBuilder&& other) noexcept = default;
    ItemBuilder& operator=(const ItemBuilder& other) noexcept = default;
    ItemBuilder& operator=(ItemBuilder&& other) noexcept = default;
    ~ItemBuilder() = default;

    explicit ItemBuilder(const XMLElement& elem) noexcept;
    ItemBuilder& Name(const std::string& name) noexcept;
    ItemBuilder& Slot(const EquipSlot& slot) noexcept;
    ItemBuilder& MinimumStats(const Stats& stats) noexcept;
    ItemBuilder& MaximumStats(const Stats& stats) noexcept;
    ItemBuilder& ParentInventory(const Inventory& parentInventory) noexcept;
    ItemBuilder& AnimateSprite(std::unique_ptr<AnimatedSprite> sprite) noexcept;
    ItemBuilder& MaxStackSize(std::size_t maximumStackSize) noexcept;

protected:
private:

    void LoadFromXml(const XMLElement& elem) noexcept;

    Inventory _parent_inventory{};
    EquipSlot _slot{};
    Stats _min_stats{};
    Stats _max_stats{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    std::string _name{};
    std::size_t _max_stack_size{ 1 };

    friend class Item;

}; //End ItemBuilder
