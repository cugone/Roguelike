#include "Game/Inventory.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

Item ItemBuilder::Build() noexcept {

}

bool Item::HasInventory() const noexcept {
    return _my_inventory != nullptr;
}

const Inventory* Item::GetInventory() const noexcept {
    return _my_inventory;
}

Inventory* Item::GetInventory() noexcept {
    return const_cast<Inventory*>(static_cast<const Item&>(*this).GetInventory());
}


Inventory::Inventory(const XMLElement& elem) noexcept
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Inventory failed to load.");
    }
}

Item* Inventory::AddItem(Item* item) noexcept {
    if(item) {
        _items.push_back(item);
        return _items.back();
    }
    return nullptr;
}

void Inventory::RemoveItem(Item* item) noexcept {
    if(item) {
        auto found_iter = std::find(std::begin(_items), std::end(_items), item);
        if(found_iter != std::end(_items)) {
            _items.erase(found_iter);
        }
    }
}

auto Inventory::size() const noexcept {
    return _items.size();
}

bool Inventory::empty() const noexcept {
    return _items.empty();
}

bool Inventory::LoadFromXml(const XMLElement& /*elem*/) {
    return false;
}
