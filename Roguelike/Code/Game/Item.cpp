#include "Game/Item.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Layer.hpp"

#include <limits>

EquipSlot EquipSlotFromString(std::string str) {
    str = StringUtils::ToLowerCase(str);
    if(str == "hair") {
        return EquipSlot::Hair;
    } else if(str == "head") {
        return EquipSlot::Head;
    } else if(str == "body") {
        return EquipSlot::Body;
    } else if(str == "larm") {
        return EquipSlot::LeftArm;
    } else if(str == "rarm") {
        return EquipSlot::RightArm;
    } else if(str == "legs") {
        return EquipSlot::Legs;
    } else if(str == "feet") {
        return EquipSlot::Feet;
    } else if(str == "cape") {
        return EquipSlot::Cape;
    } else {
        return EquipSlot::None;
    }
}

std::string EquipSlotToString(const EquipSlot& slot) {
    switch(slot) {
    case EquipSlot::Hair: return "hair";
    case EquipSlot::Head: return "head";
    case EquipSlot::Body: return "body";
    case EquipSlot::LeftArm: return "larm";
    case EquipSlot::RightArm: return "rarm";
    case EquipSlot::Legs: return "legs";
    case EquipSlot::Feet: return "feet";
    case EquipSlot::Cape: return "cape";
    default: return "none";
    }
}


std::map<std::string, std::unique_ptr<Item>> Item::s_registry{};

Item* Item::CreateItem(ItemBuilder& builder) {
    auto new_item = std::make_unique<Item>(builder);
    auto new_item_name = new_item->GetName();
    auto ptr = new_item.get();
    s_registry.emplace(new_item_name, std::move(new_item));
    return ptr;
}

void Item::ClearItemRegistry() {
    s_registry.clear();
}

Item::Item(ItemBuilder& builder) noexcept
    : _name(builder._name)
    , _friendly_name(builder._friendly_name)
    , _sprite(std::move(builder._sprite))
    , _slot(builder._slot)
    , _max_stack_size(builder._max_stack_size)
    , _light_value(builder._light_value)
{
    if(_friendly_name.empty()) {
        _friendly_name = _name;
        _friendly_name = StringUtils::ReplaceAll(_friendly_name, "_", " ");
    }
    for(auto stat_id = StatsID::First_; stat_id < StatsID::Last_; ++stat_id) {
        _stat_modifiers.SetStat(stat_id, MathUtils::GetRandomInRange(builder._min_stats.GetStat(stat_id), builder._max_stats.GetStat(stat_id)));
    }
}

void Item::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(_sprite) {
        _sprite->Update(deltaSeconds);
    }
}

bool Item::HasOwningInventory() const noexcept {
    return _parent_inventory != nullptr;
}

bool Item::IsChildInventory() const noexcept {
    return HasOwningInventory();
}

const Inventory& Item::GetInventory() const noexcept {
    return _my_inventory;
}

Inventory& Item::GetInventory() noexcept {
    return _my_inventory;
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
        return found_iter->second.get();
    }
    return nullptr;
}

const AnimatedSprite* Item::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* Item::GetSprite() {
    return _sprite.get();
}

std::string Item::GetName() const noexcept {
    return _name;
}

std::string Item::GetFriendlyName() const noexcept {
    return _friendly_name;
}

const Inventory* Item::OwningInventory() const noexcept {
    return _parent_inventory;
}

Inventory* Item::OwningInventory() noexcept {
    return _parent_inventory;
}

std::size_t Item::GetCount() const noexcept {
    return _stack_size;
}

std::size_t Item::IncrementCount() noexcept {
    ++_stack_size;
    if(_max_stack_size) {
        _stack_size = std::clamp(_stack_size, std::size_t{0}, _max_stack_size);
    }
    return _stack_size;
}

std::size_t Item::DecrementCount() noexcept {
    if(_stack_size) {
        --_stack_size;
    }
    return _stack_size;
}

void Item::AdjustCount(long long amount) noexcept {
    if(amount < 0 && _stack_size <= static_cast<std::size_t>(-amount)) {
        _stack_size = 0u;
    } else {
        _stack_size += static_cast<std::size_t>(amount);
        if(_max_stack_size > 0) {
            _stack_size = std::clamp(_stack_size, std::size_t{0}, _max_stack_size);
        }
    }
}

void Item::SetCount(std::size_t newCount) noexcept {
    _stack_size = newCount;
    if(_max_stack_size) {
        _stack_size = std::clamp(_stack_size, std::size_t{0}, _max_stack_size);
    }
}


const EquipSlot& Item::GetEquipSlot() const {
    return _slot;
}

const uint32_t Item::GetLightValue() const noexcept {
    return _light_value;
}

ItemBuilder::ItemBuilder(const XMLElement& elem, std::weak_ptr<SpriteSheet> itemSheet) noexcept
    : _itemSheet(itemSheet)
{
    LoadFromXml(elem, itemSheet);
}

ItemBuilder& ItemBuilder::Name(const std::string& name) noexcept {
    _name = name;
    return *this;
}

ItemBuilder& ItemBuilder::FriendlyName(const std::string& friendlyName) noexcept {
    _friendly_name = friendlyName;
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

ItemBuilder& ItemBuilder::LightValue(uint32_t value) noexcept {
    _light_value = std::clamp(value, uint32_t{min_light_value}, uint32_t{max_light_value});
    return *this;
}

Item* ItemBuilder::Build() noexcept {
    auto item = Item::CreateItem(*this);
    return item;
}

void ItemBuilder::LoadFromXml(const XMLElement& elem, std::weak_ptr<SpriteSheet> itemSheet) noexcept {
    DataUtils::ValidateXmlElement(elem, "item", "", "name", "stats,equipslot,animation", "index,maxstack,light");
    const auto name = DataUtils::ParseXmlAttribute(elem, "name", std::string{"UNKNOWN ITEM"});
    Name(name);
    if(auto* xml_equipslot = elem.FirstChildElement("equipslot")) {
        Slot(EquipSlotFromString(DataUtils::ParseXmlElementText(*xml_equipslot, std::string{"none"})));
    } else {
        Slot(EquipSlot::None);
    }
    if(auto* xml_minstats = elem.FirstChildElement("stats")) {
        MinimumStats(Stats(*xml_minstats));
        MaximumStats(Stats(*xml_minstats));
        if(auto* xml_maxstats = xml_minstats->NextSiblingElement("stats")) {
            MaximumStats(Stats(*xml_maxstats));
        }
    }
    if(DataUtils::HasAttribute(elem, "index")) {
        auto startIndex = DataUtils::ParseXmlAttribute(elem, "index", IntVector2::One * -1);
        if(auto* xml_animsprite = elem.FirstChildElement("animation")) {
            if(!_itemSheet.expired()) {
                auto s = _itemSheet.lock();
                AnimateSprite(g_theRenderer->CreateAnimatedSprite(s, *xml_animsprite));
            }
        } else {
            if (!_itemSheet.expired()) {
                auto s = _itemSheet.lock();
                AnimateSprite(g_theRenderer->CreateAnimatedSprite(s, startIndex));
            }
        }
    }
    MaxStackSize(DataUtils::ParseXmlAttribute(elem, "maxstack", _max_stack_size));
    if(LightValue(0); DataUtils::HasChild(elem, "light")) {
        if(auto* xml_light = elem.FirstChildElement("light")) {
            LightValue(DataUtils::ParseXmlAttribute(*xml_light, "value", uint32_t{0}));
        }
    }
}

