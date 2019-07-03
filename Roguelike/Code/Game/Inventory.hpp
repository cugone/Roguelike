#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <vector>

class ItemBuilder;
class Item;
class Inventory;


#include "Engine/Core/DataUtils.hpp"

#include "Game/Inventory.hpp"

#include <map>
#include <memory>
#include <string>

class ItemBuilder;
class Item;
class Inventory;

class ItemBuilder {

    ItemBuilder() = default;
    ItemBuilder(const ItemBuilder& other) noexcept = default;
    ItemBuilder(ItemBuilder&& other) noexcept = default;
    ItemBuilder& operator=(const ItemBuilder& other) noexcept = default;
    ItemBuilder& operator=(ItemBuilder&& other) noexcept = default;
    ~ItemBuilder() = default;

    ItemBuilder(const XMLElement& elem);

    Item Build() noexcept;

};

class Item {
public:

    static std::map<std::string, std::unique_ptr<Item>> s_registry;

    Item() = default;
    Item(const Item& other) noexcept = default;
    Item(Item&& other) noexcept = default;
    Item& operator=(const Item& other) noexcept = default;
    Item& operator=(Item&& other) noexcept = default;
    ~Item() = default;

    Item(const ItemBuilder& builder) noexcept;

    bool HasInventory() const noexcept;
    const Inventory* GetInventory() const noexcept;
    Inventory* GetInventory() noexcept;

protected:
private:
    Inventory _parent_inventory{};
    Inventory* _my_inventory{};
};


class Inventory {
public:
    Inventory() = default;
    Inventory(const Inventory& other) noexcept = default;
    Inventory(Inventory&& other) noexcept = default;
    Inventory& operator=(const Inventory& other) noexcept = default;
    Inventory& operator=(Inventory&& other) noexcept = default;
    ~Inventory() = default;

    Inventory(const XMLElement& elem) noexcept;

    Item* AddItem(Item* item) noexcept;
    void RemoveItem(Item* item) noexcept;

    auto size() const noexcept;
    bool empty() const noexcept;

protected:
private:
    bool LoadFromXml(const XMLElement& elem);

    std::vector<Item*> _items{};
};

