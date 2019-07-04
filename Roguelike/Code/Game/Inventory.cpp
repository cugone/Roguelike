#include "Game/Inventory.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Game/GameCommon.hpp"

#include <utility>

ItemBuilder::ItemBuilder(const XMLElement& elem) noexcept
{
    LoadFromXml(elem);
}

ItemBuilder& ItemBuilder::Name(const std::string& name) noexcept {
    _name = name;
    return *this;
}

ItemBuilder& ItemBuilder::Slot(const EquipSlot& slot) noexcept {
    _slot = slot;
    return *this;
}

ItemBuilder& ItemBuilder::MinimumStats(const Stats& stats) noexcept {
    _min_stats = stats;
    return *this;
}

ItemBuilder& ItemBuilder::MaximumStats(const Stats& stats) noexcept {
    _max_stats = stats;
    return *this;
}

ItemBuilder& ItemBuilder::ParentInventory(const Inventory& parentInventory) noexcept {
    _parent_inventory = parentInventory;
    return *this;
}

ItemBuilder& ItemBuilder::AnimateSprite(std::unique_ptr<AnimatedSprite> sprite) noexcept {
    _sprite = std::move(sprite);
    return *this;
}

ItemBuilder& ItemBuilder::MaxStackSize(std::size_t maximumStackSize) noexcept {
    _max_stack_size = maximumStackSize;
    return *this;
}

Item* ItemBuilder::Build() noexcept {
    return new Item(*this);
}

void ItemBuilder::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "item", "animation,equipslot,stats", "maxstack");
    if(auto* xml_animation = elem.FirstChildElement("animation")) {
        _sprite = std::move(g_theRenderer->CreateAnimatedSprite(*xml_animation));
    }
    if(auto* xml_equipslot = elem.FirstChildElement("equipslot")) {
        DataUtils::ValidateXmlElement(*xml_equipslot, "equipslot", "", "slot");
        auto slot_str = DataUtils::ParseXmlAttribute(*xml_equipslot, "slot", "none");
        _slot = EquipSlotFromString(slot_str);
    }
    if(auto* xml_minstats = elem.FirstChildElement("stats")) {
        _min_stats = Stats(*xml_minstats);
        _max_stats = Stats(*xml_minstats);
        if(auto* xml_maxstats = xml_minstats->NextSiblingElement("stats")) {
            _max_stats = Stats(*xml_maxstats);
        }
    }
    _max_stack_size = DataUtils::ParseXmlAttribute(elem, "maxstack", _max_stack_size);
}

std::map<std::string, Item*> Item::s_registry{};

Item::Item(ItemBuilder& builder) noexcept
    : _name(builder._name)
    , _sprite(std::move(builder._sprite))
    , _max_stack_size(builder._max_stack_size)
{
    for(auto stat_id = StatsID::First_; stat_id < StatsID::Last_; ++stat_id) {
        _stat_modifiers.SetStat(stat_id, MathUtils::GetRandomLongLongInRange(builder._min_stats.GetStat(stat_id), builder._max_stats.GetStat(stat_id)));
    }
    s_registry.try_emplace(_name, this);
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


const Stats Item::GetStatModifiers() const {
    return _stat_modifiers;
}

Stats Item::GetStatModifiers() {
    return static_cast<const Item&>(*this).GetStatModifiers();
}

Item* Item::GetItem(const std::string& name) {
    auto found_iter = s_registry.find(name);
    if(found_iter != std::end(s_registry)) {
        return found_iter->second;
    }
    return nullptr;
}

const AnimatedSprite* Item::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* Item::GetSprite() {
    return const_cast<AnimatedSprite*>(static_cast<const Item&>(*this).GetSprite());
}

std::string Item::GetName() const noexcept {
    return _name;
}

const Inventory& Item::OwningInventory() const noexcept {
    return _parent_inventory;
}

Inventory& Item::OwningInventory() noexcept {
    return const_cast<Inventory&>(static_cast<const Item&>(*this).OwningInventory());
}

std::size_t Item::GetCount() const noexcept {
    return _stack_size;
}

void Item::IncrementCount() noexcept {
    ++_stack_size;
    _stack_size = std::clamp(_stack_size, std::size_t{ 0 }, _max_stack_size);
}

void Item::DecrementCount() noexcept {
    if(_stack_size == 0) {
        return;
    }
    --_stack_size;
}

void Item::SetCount(std::size_t newCount) noexcept {
    _stack_size = newCount;
}

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
    auto found_iter = std::find_if(std::begin(_items), std::end(_items), [&name](const Item* item) { return item->GetName() == name; });
    if(found_iter != std::end(_items)) {
        return *found_iter;
    }
    return nullptr;
}

Item* Inventory::AddItem(Item* item) noexcept {
    if(item) {
        if(auto i = HasItem(item)) {
            i->IncrementCount();
            return i;
        } else {
            _items.push_back(item);
            return _items.back();
        }
    }
    return nullptr;
}

void Inventory::RemoveItem(Item* item) noexcept {
    if(item) {
        if(auto i = HasItem(item)) {
            i->DecrementCount();
            return;
        }
        auto found_iter = std::find(std::begin(_items), std::end(_items), item);
        if(found_iter != std::end(_items)) {
            _items.erase(found_iter);
        }
    }
}

void Inventory::RemoveItem(const std::string& name) noexcept {
    auto found_iter = std::find_if(std::begin(_items), std::end(_items), [&name](Item* item) { return item->GetName() == name;  });
    RemoveItem(found_iter != std::end(_items) ? *found_iter : nullptr);
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
        _items.emplace_back((ItemBuilder(elem).Build()));
    });
}
