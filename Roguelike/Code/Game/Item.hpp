#pragma once

#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/Inventory.hpp"
#include "Game/Stats.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace a2de {
    class AnimatedSprite;
    class Inventory;
    class Layer;
}

class ItemBuilder;

//Also represents render order.
enum class EquipSlot {
    None
    , Hair
    , Head
    , LeftArm
    , RightArm
    , Feet
    , Legs
    , Body
    , Cape
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

    void Update(a2de::TimeUtils::FPSeconds deltaSeconds, const a2de::Vector2& position, Layer* parent_layer);
    void AddVerts(const a2de::Vector2& position, Layer* parent_layer) const;

    const Inventory& GetInventory() const noexcept;
    Inventory& GetInventory() noexcept;

    const Stats GetStatModifiers() const;
    Stats GetStatModifiers();

    static Item* GetItem(const std::string& name);

    const a2de::AnimatedSprite* GetSprite() const;
    a2de::AnimatedSprite* GetSprite();

    std::string GetName() const noexcept;
    std::string GetFriendlyName() const noexcept;

    bool HasOwningInventory() const noexcept;
    bool IsChildInventory() const noexcept;
    const Inventory* OwningInventory() const noexcept;
    Inventory* OwningInventory() noexcept;

    std::size_t GetCount() const noexcept;
    std::size_t IncrementCount() noexcept;
    std::size_t DecrementCount() noexcept;
    void AdjustCount(long long amount) noexcept;
    void SetCount(std::size_t newCount) noexcept;

    const EquipSlot& GetEquipSlot() const;

protected:
private:
    std::string _name{};
    std::string _friendly_name{};
    std::unique_ptr<a2de::AnimatedSprite> _sprite{};
    Inventory* _parent_inventory{};
    Inventory _my_inventory{};
    Stats _stat_modifiers{0};
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

    explicit ItemBuilder(const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> itemSheet) noexcept;
    ItemBuilder& Name(const std::string& name) noexcept;
    ItemBuilder& FriendlyName(const std::string& friendlyName) noexcept;
    ItemBuilder& Slot(const EquipSlot& slot) noexcept;
    ItemBuilder& MinimumStats(const Stats& stats) noexcept;
    ItemBuilder& MaximumStats(const Stats& stats) noexcept;
    ItemBuilder& ParentInventory(const Inventory& parentInventory) noexcept;
    ItemBuilder& AnimateSprite(std::unique_ptr<a2de::AnimatedSprite> sprite) noexcept;
    ItemBuilder& MaxStackSize(std::size_t maximumStackSize) noexcept;

protected:
private:

    void LoadFromXml(const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> itemSheet) noexcept;

    Inventory _parent_inventory{};
    EquipSlot _slot{};
    Stats _min_stats{};
    Stats _max_stats{};
    std::unique_ptr<a2de::AnimatedSprite> _sprite{};
    std::weak_ptr<a2de::SpriteSheet> _itemSheet{};
    std::string _name{};
    std::string _friendly_name{};
    std::size_t _max_stack_size{ 1 };

    friend class Item;

}; //End ItemBuilder
