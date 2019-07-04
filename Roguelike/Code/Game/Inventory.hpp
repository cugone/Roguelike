#pragma once

#include "Engine/Core/DataUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/EntityDefinition.hpp"
#include "Game/Equipment.hpp"
#include "Game/Stats.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class AnimatedSprite;
class ItemBuilder;
class Item;
class Inventory;

class Inventory {
private:
    std::vector<Item*> _items{};
public:

    using value_type = decltype(_items)::value_type;
    using allocator_type = decltype(_items)::allocator_type;
    using pointer = decltype(_items)::pointer;
    using const_pointer = decltype(_items)::const_pointer;
    using reference = decltype(_items)::reference;
    using const_reference = decltype(_items)::const_reference;
    using size_type = decltype(_items)::size_type;
    using difference_type = decltype(_items)::difference_type;
    using iterator = decltype(_items)::iterator;
    using const_iterator = decltype(_items)::const_iterator;
    using reverse_iterator = decltype(_items)::reverse_iterator;
    using const_reverse_iterator = decltype(_items)::const_reverse_iterator;

    Inventory() = default;
    Inventory(const Inventory& other) = default;
    Inventory(Inventory&& other) = default;
    Inventory& operator=(const Inventory& other) = default;
    Inventory& operator=(Inventory&& other) = default;
    ~Inventory() = default;

    explicit Inventory(const XMLElement& elem) noexcept;

    Item* HasItem(Item* item) const noexcept;
    Item* HasItem(const std::string& name) const noexcept;

    Item* AddItem(Item* item) noexcept;
    void RemoveItem(Item* item) noexcept;
    void RemoveItem(const std::string& name) noexcept;

    const Item* GetItem(const std::string& name) const noexcept;
    const Item* GetItem(std::size_t idx) const noexcept;
    Item* GetItem(const std::string& name) noexcept;
    Item* GetItem(std::size_t idx) noexcept;

    static bool TransferItem(Inventory& source, Inventory& dest, Item* item) noexcept;
    bool TransferItem(Inventory& dest, Item* item) noexcept;
    static bool TransferItem(Inventory& source, Inventory& dest, const std::string& name) noexcept;
    bool TransferItem(Inventory& dest, const std::string& name) noexcept;
    static void TransferAll(Inventory& source, Inventory& dest) noexcept;
    void TransferAll(Inventory& dest) noexcept;

    auto size() const noexcept;
    bool empty() const noexcept;
    void clear() noexcept;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;

    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    reverse_iterator rbegin() noexcept;
    reverse_iterator rend() noexcept;
    const_reverse_iterator rbegin() const noexcept;
    const_reverse_iterator rend() const noexcept;

    const_reverse_iterator crbegin() const noexcept;
    const_reverse_iterator crend() const noexcept;

protected:
private:
    void LoadFromXml(const XMLElement& elem);
    
}; //End Inventory

class Item {
public:

    static std::map<std::string, Item*> s_registry;

    Item(const Item& other) = default;
    Item(Item&& other) = default;
    Item& operator=(const Item& other)= default;
    Item& operator=(Item&& other) = default;
    ~Item() = default;

    explicit Item(ItemBuilder& builder) noexcept;

    bool HasInventory() const noexcept;
    const Inventory* GetInventory() const noexcept;
    Inventory* GetInventory() noexcept;

    const Stats GetStatModifiers() const;
    Stats GetStatModifiers();

    Item* GetItem(const std::string& name);

    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();

    std::string GetName() const noexcept;

    const Inventory& OwningInventory() const noexcept;
    Inventory& OwningInventory() noexcept;

    std::size_t GetCount() const noexcept;
    void IncrementCount() noexcept;
    void DecrementCount() noexcept;
    void SetCount(std::size_t newCount) noexcept;

protected:
private:
    std::string _name{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    Inventory _parent_inventory{};
    Inventory* _my_inventory{};
    Stats _stat_modifiers{};
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
    std::size_t _max_stack_size{1};

    friend class Item;

}; //End ItemBuilder
