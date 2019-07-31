#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Renderer/AnimatedSprite.hpp"

#include "Game/Stats.hpp"

#include <bitset>
#include <map>
#include <memory>

class AnimatedSprite;
class SpriteSheet;
class Renderer;
class EquipmentDefinition;

class EntityDefinition {
public:

    enum class AttachPoint {
        None
        ,Hair
        ,Head
        ,Body
        ,LeftArm
        ,RightArm
        ,LeftLeg
        ,RightLeg
        ,Feet
        ,Max
    };

    static void CreateEntityDefinition(Renderer& renderer, const XMLElement& elem);
    static void CreateEntityDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet);
    static void DestroyEntityDefinitions();
    static EntityDefinition* GetEntityDefinitionByName(const std::string& name);
    static void ClearEntityRegistry();
    static std::vector<std::string> GetAllEntityDefinitionNames();

    EntityDefinition() = delete;
    EntityDefinition(const EntityDefinition& other) = default;
    EntityDefinition(EntityDefinition&& other) = default;
    EntityDefinition& operator=(const EntityDefinition& other) = default;
    EntityDefinition& operator=(EntityDefinition&& other) = default;
    ~EntityDefinition() = default;

    EntityDefinition(Renderer& renderer, const XMLElement& elem);
    EntityDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet);

    std::string name{"UNKNOWN ENTITY"};
    bool is_invisible = false;
    bool is_animated = false;

    void SetBaseStats(const Stats& newBaseStats) noexcept;
    const Stats& GetBaseStats() const noexcept;
    Stats& GetBaseStats() noexcept;

    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();

    bool HasAttachPoint(const AttachPoint& attachpoint);
    Vector2 GetAttachPoint(const AttachPoint& attachpoint);

protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    void LoadAnimation(const XMLElement &elem);
    void LoadAttachPoints(const XMLElement &elem);
    void LoadEquipment(const XMLElement& elem);
    void LoadStats(const XMLElement& elem);

    static std::map<std::string, std::unique_ptr<EntityDefinition>> s_registry;
    Renderer& _renderer;
    std::shared_ptr<SpriteSheet> _sheet{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    std::vector<Vector2> attach_point_offsets{};
    std::vector<EquipmentDefinition*> equipments{};
    IntVector2 _index{};
    Stats _base_stats{};
    std::bitset<static_cast<std::size_t>(AttachPoint::Max)> _valid_offsets{};
};

using EquipSlot = EntityDefinition::AttachPoint;

EquipSlot EquipSlotFromString(std::string str);
std::string EquipSlotToString(const EquipSlot& slot);
