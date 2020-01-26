#include "Game/Item.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/GameCommon.hpp"

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
{
    if (_friendly_name.empty()) {
        _friendly_name = _name;
        _friendly_name = StringUtils::ReplaceAll(_friendly_name, "_", " ");
    }
    for(auto stat_id = StatsID::First_; stat_id < StatsID::Last_; ++stat_id) {
        _stat_modifiers.SetStat(stat_id, MathUtils::GetRandomLongInRange(builder._min_stats.GetStat(stat_id), builder._max_stats.GetStat(stat_id)));
    }
}

void Item::Update(TimeUtils::FPSeconds deltaSeconds) {
    if(_sprite) {
        _sprite->Update(deltaSeconds);
    }
}

void Item::Render(const IntVector2& entity_position, std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const {

    if(!_sprite) {
        return;
    }

    const auto position = entity_position;
    const auto color = Rgba::White;

    const auto& coords = _sprite->GetCurrentTexCoords();

    const auto vert_left = position.x + 0.0f;
    const auto vert_right = position.x + 1.0f;
    const auto vert_top = position.y + 0.0f;
    const auto vert_bottom = position.y + 1.0f;

    const auto vert_bl = Vector2(vert_left, vert_bottom);
    const auto vert_tl = Vector2(vert_left, vert_top);
    const auto vert_tr = Vector2(vert_right, vert_top);
    const auto vert_br = Vector2(vert_right, vert_bottom);

    const auto tx_left = coords.mins.x;
    const auto tx_right = coords.maxs.x;
    const auto tx_top = coords.mins.y;
    const auto tx_bottom = coords.maxs.y;

    const auto tx_bl = Vector2(tx_left, tx_bottom);
    const auto tx_tl = Vector2(tx_left, tx_top);
    const auto tx_tr = Vector2(tx_right, tx_top);
    const auto tx_br = Vector2(tx_right, tx_bottom);

    const float z = static_cast<float>(layer_index);
    verts.push_back(Vertex3D(Vector3(vert_bl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_bl));
    verts.push_back(Vertex3D(Vector3(vert_tl, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tl));
    verts.push_back(Vertex3D(Vector3(vert_tr, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_tr));
    verts.push_back(Vertex3D(Vector3(vert_br, z), layer_color != color && color != Rgba::White ? color : layer_color, tx_br));

    const auto v_s = verts.size();
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 3u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 4u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 2u);
    ibo.push_back(static_cast<unsigned int>(v_s) - 1u);

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
    _stack_size = std::clamp(_stack_size, std::size_t{ 0 }, _max_stack_size);
    return _stack_size;
}

std::size_t Item::DecrementCount() noexcept {
    if(_stack_size) {
        --_stack_size;
    }
    return _stack_size;
}

void Item::AdjustCount(long long amount) noexcept {
    if (amount < 0 && _stack_size <= static_cast<std::size_t>(-amount)) {
        _stack_size = 0u;
    } else {
        _stack_size += static_cast<std::size_t>(amount);
        _stack_size = std::clamp(_stack_size, std::size_t{ 0 }, _max_stack_size);
    }
}

void Item::SetCount(std::size_t newCount) noexcept {
    _stack_size = newCount;
    _stack_size = std::clamp(_stack_size, std::size_t{ 0 }, _max_stack_size);
}


const EquipSlot& Item::GetEquipSlot() const {
    return _slot;
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

Item* ItemBuilder::Build() noexcept {
    auto item = Item::CreateItem(*this);
    return item;
}

void ItemBuilder::LoadFromXml(const XMLElement& elem, std::weak_ptr<SpriteSheet> itemSheet) noexcept {
    DataUtils::ValidateXmlElement(elem, "item", "", "name", "stats,equipslot,animation", "index,maxstack");
    const auto name = DataUtils::ParseXmlAttribute(elem, "name", "UNKNOWN ITEM");
    Name(name);
    if(auto* xml_equipslot = elem.FirstChildElement("equipslot")) {
        Slot(EquipSlotFromString(DataUtils::ParseXmlElementText(*xml_equipslot, "none")));
    } else {
        Slot(EquipSlot::None);
    }
    if(auto* xml_minstats = elem.FirstChildElement("stats")) {
        MinimumStats(Stats(*xml_minstats));
        MaximumStats(Stats(*xml_minstats));
        if(auto* xml_maxstats = xml_minstats->NextSiblingElement("stats")) {
            MaximumStats(Stats(*xml_minstats));
        }
    }
    if(DataUtils::HasAttribute(elem, "index")) {
        auto startIndex = DataUtils::ParseXmlAttribute(elem, "index", IntVector2::ONE * -1);
        if(auto* xml_animsprite = elem.FirstChildElement("animation")) {
            AnimateSprite(g_theRenderer->CreateAnimatedSprite(_itemSheet, *xml_animsprite));
        } else {
            AnimateSprite(g_theRenderer->CreateAnimatedSprite(_itemSheet, startIndex));
        }
    }
    MaxStackSize(DataUtils::ParseXmlAttribute(elem, "maxstack", _max_stack_size));
}

