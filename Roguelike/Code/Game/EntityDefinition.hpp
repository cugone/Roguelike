#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/Inventory.hpp"
#include "Game/Stats.hpp"
#include "Game/Item.hpp"

#include <bitset>
#include <map>
#include <memory>

class AnimatedSprite;
class SpriteSheet;
class Renderer;
class Behavior;

class EntityDefinition {
public:

    enum class AttachPoint {
        None
        ,Hair
        ,Head
        ,Body
        ,LeftArm
        ,RightArm
        ,Legs
        ,Feet
        ,Cape
        ,Max
    };

    static void CreateEntityDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem);
    static void CreateEntityDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::shared_ptr<a2de::SpriteSheet> sheet);
    static EntityDefinition* GetEntityDefinitionByName(const std::string& name);
    static void ClearEntityRegistry();
    static std::vector<std::string> GetAllEntityDefinitionNames();

    EntityDefinition() = delete;
    EntityDefinition(const EntityDefinition& other) = default;
    EntityDefinition(EntityDefinition&& other) = default;
    EntityDefinition& operator=(const EntityDefinition& other) = default;
    EntityDefinition& operator=(EntityDefinition&& other) = default;
    ~EntityDefinition() = default;

    EntityDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem);
    EntityDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::shared_ptr<a2de::SpriteSheet> sheet);

    std::string name{"UNKNOWN ENTITY"};
    bool is_invisible = false;
    bool is_animated = false;
    Inventory inventory{};
    std::vector<Item*> equipment = std::vector<Item*>(static_cast<std::size_t>(EquipSlot::Max));

    void SetBaseStats(const Stats& newBaseStats) noexcept;
    const Stats& GetBaseStats() const noexcept;
    Stats& GetBaseStats() noexcept;

    const a2de::AnimatedSprite* GetSprite() const;
    a2de::AnimatedSprite* GetSprite();

    bool HasAttachPoint(const AttachPoint& attachpoint);
    a2de::Vector2 GetAttachPoint(const AttachPoint& attachpoint);

    const std::vector<std::shared_ptr<class Behavior>>& GetAvailableBehaviors() const noexcept;

protected:
private:
    bool LoadFromXml(const a2de::XMLElement& elem);
    void LoadAnimation(const a2de::XMLElement &elem);
    void LoadAttachPoints(const a2de::XMLElement &elem);
    void LoadStats(const a2de::XMLElement& elem);
    void LoadInventory(const a2de::XMLElement& elem);
    void LoadEquipment(const a2de::XMLElement& elem);
    void LoadBehaviors(const a2de::XMLElement& elem);

    static std::map<std::string, std::unique_ptr<EntityDefinition>> s_registry;
    a2de::Renderer& _renderer;
    std::shared_ptr<class a2de::SpriteSheet> _sheet{};
    std::unique_ptr<class a2de::AnimatedSprite> _sprite{};
    std::vector<std::shared_ptr<class Behavior>> _available_behaviors{};
    std::vector<a2de::Vector2> attach_point_offsets{};
    a2de::IntVector2 _index{};
    Stats _base_stats{};
    std::bitset<static_cast<std::size_t>(AttachPoint::Max)> _valid_offsets{};
};
