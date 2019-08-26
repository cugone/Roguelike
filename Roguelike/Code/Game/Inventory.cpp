#include "Game/Inventory.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Item.hpp"

#include <utility>

Inventory::Inventory(const XMLElement& elem) noexcept
{
    LoadFromXml(elem);
}

Item* Inventory::HasItem(Item* item) const noexcept {
    if(!item) {
        return nullptr;
    }
    auto found_iter = std::find(std::begin(_items), std::end(_items), item);
    if(found_iter != std::end(_items)) {
        return *found_iter;
    }
    return nullptr;
}

Item* Inventory::HasItem(const std::string& name) const noexcept {
    auto found_iter = std::find_if(std::begin(_items), std::end(_items), [&name](const Item* item) { return StringUtils::ToLowerCase(item->GetName()) == StringUtils::ToLowerCase(name); });
    if(found_iter != std::end(_items)) {
        return *found_iter;
    }
    return nullptr;
}

void Inventory::AddStack(Item* item, std::size_t count) noexcept {
    if(auto* i = HasItem(item)) {
        i->AdjustCount(count);
    } else {
        i = AddItem(item);
        i->SetCount(count);
    }
}

void Inventory::AddStack(const std::string& name, std::size_t count) noexcept {
    if(auto* item = HasItem(name)) {
        item->AdjustCount(count);
    } else {
        if((item = AddItem(name)) != nullptr) {
            item->SetCount(count);
        }
    }
}

Item* Inventory::AddItem(Item* item) noexcept {
    if(item) {
        if(auto i = HasItem(item)) {
            i->IncrementCount();
            return i;
        } else {
            _items.push_back(item);
            _items.back()->IncrementCount();
            return _items.back();
        }
    }
    return nullptr;
}

Item* Inventory::AddItem(const std::string& name) noexcept {
    if(auto* item_in_inventory = HasItem(name)) {
        item_in_inventory->IncrementCount();
        return item_in_inventory;
    } else {
        if (auto* item_in_registry = Item::GetItem(name)) {
            _items.push_back(item_in_registry);
            _items.back()->IncrementCount();
            return _items.back();
        }
    }
    return nullptr;
}

void Inventory::RemoveItem(Item* item) noexcept {
    if(item) {
        if(auto i = HasItem(item)) {
            if(!i->DecrementCount()) {
                auto found_iter = std::find(std::begin(_items), std::end(_items), i);
                if(found_iter != std::end(_items)) {
                    _items.erase(found_iter);
                }
            }
        }
    }
}

void Inventory::RemoveItem(const std::string& name) noexcept {
    if(auto i = HasItem(name)) {
        if(!i->DecrementCount()) {
            auto found_iter = std::find(std::begin(_items), std::end(_items), i);
            if(found_iter != std::end(_items)) {
                _items.erase(found_iter);
            }
        }
    }
}

const Item* Inventory::GetItem(const std::string& name) const noexcept {
    auto found_iter = std::find_if(std::begin(_items), std::end(_items), [&name](const Item* item)->bool { return item->GetName() == name; });
    if(found_iter == std::end(_items)) {
        return nullptr;
    }
    return *found_iter;
}

const Item* Inventory::GetItem(std::size_t idx) const noexcept {
    if(_items.size() < idx) {
        return nullptr;
    }
    return _items[idx];
}

Item* Inventory::GetItem(const std::string& name) noexcept {
    return const_cast<Item*>(static_cast<const Inventory&>(*this).GetItem(name));
}

Item* Inventory::GetItem(std::size_t idx) noexcept {
    return const_cast<Item*>(static_cast<const Inventory&>(*this).GetItem(idx));
}

bool Inventory::TransferItem(Inventory& source, Inventory& dest, Item* item) noexcept {
    source.RemoveItem(item);
    return dest.AddItem(item) != nullptr;
}

bool Inventory::TransferItem(Inventory& dest, Item* item) noexcept {
    return Inventory::TransferItem(*this, dest, item);
}

bool Inventory::TransferItem(Inventory& source, Inventory& dest, const std::string& name) noexcept {
    auto item = source.GetItem(name);
    source.RemoveItem(item);
    return dest.AddItem(item) != nullptr;
}

bool Inventory::TransferItem(Inventory& dest, const std::string& name) noexcept {
    return Inventory::TransferItem(*this, dest, name);
}

void Inventory::TransferAll(Inventory& source, Inventory& dest) noexcept {
    bool success = false;
    for(const auto& item : source) {
        success |= (dest.AddItem(item) != nullptr);
    }
    source.clear();
}

void Inventory::TransferAll(Inventory& dest) noexcept {
    return Inventory::TransferAll(*this, dest);
}

auto Inventory::size() const noexcept {
    return _items.size();
}

bool Inventory::empty() const noexcept {
    return _items.empty();
}

void Inventory::clear() noexcept {
    _items.clear();
}

Inventory::iterator Inventory::begin() noexcept {
    return _items.begin();
}
Inventory::iterator Inventory::end() noexcept {
    return _items.end();
}

Inventory::const_iterator Inventory::begin() const noexcept {
    return _items.begin();
}
Inventory::const_iterator Inventory::end() const noexcept {
    return _items.end();
}

Inventory::const_iterator Inventory::cbegin() const noexcept {
    return _items.cbegin();
}
Inventory::const_iterator Inventory::cend() const noexcept {
    return _items.cend();
}

Inventory::reverse_iterator Inventory::rbegin() noexcept {
    return _items.rbegin();
}
Inventory::reverse_iterator Inventory::rend() noexcept {
    return _items.rend();
}

Inventory::const_reverse_iterator Inventory::rbegin() const noexcept {
    return _items.rbegin();
}
Inventory::const_reverse_iterator Inventory::rend() const noexcept {
    return _items.rend();
}

Inventory::const_reverse_iterator Inventory::crbegin() const noexcept {
    return _items.crbegin();
}
Inventory::const_reverse_iterator Inventory::crend() const noexcept {
    return _items.crend();
}


void Inventory::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "inventory", "item", "");
    DataUtils::ForEachChildElement(elem, "item",
    [this](const XMLElement& elem) {
        auto item_name = DataUtils::ParseXmlAttribute(elem, "name", "");
        if(auto item = Item::GetItem(item_name)) {
            _items.emplace_back(item);
        }
    });
}
