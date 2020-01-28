#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <string>
#include <vector>

class Item;

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

    void AddStack(Item* item, std::size_t count) noexcept;
    void AddStack(const std::string& name, std::size_t count) noexcept;
    Item* AddItem(Item* item) noexcept;
    Item* AddItem(const std::string& name) noexcept;

    void RemoveItem(Item* item) noexcept;
    void RemoveItem(Item* item, std::size_t count) noexcept;
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

    std::size_t size() const noexcept;
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
