#pragma once

#include "Engine/Math/IntVector2.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Event.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Game/Inventory.hpp"
#include "Game/Stats.hpp"

namespace a2de {
    class AnimatedSprite;
}

class Map;
class Layer;
class Tile;
class EntityDefinition;


enum class Faction {
    None
    ,Player
    ,Enemy
    ,Neutral
};

class Entity {
public:
    Entity();
    Entity(const Entity& other) = default;
    Entity(Entity&& other) = default;
    Entity& operator=(const Entity& other) = default;
    Entity& operator=(Entity&& other) = default;
    virtual ~Entity() = 0;

    Entity(EntityDefinition* definition) noexcept;
    Entity(const a2de::XMLElement& elem) noexcept;

    virtual void BeginFrame();
    virtual void Update(a2de::TimeUtils::FPSeconds deltaSeconds);
    virtual void EndFrame();

    static void Fight(Entity& attacker, Entity& defender);

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    virtual void SetPosition(const a2de::IntVector2& position);
    const a2de::IntVector2& GetPosition() const;
    const a2de::Vector2& GetScreenPosition() const;

    Stats GetStats() const;
    void AdjustBaseStats(Stats adjustments);
    void AdjustStatModifiers(Stats adjustments);

    Faction GetFaction() const noexcept;
    void SetFaction(const Faction& faction) noexcept;
    Faction JoinFaction(const Faction& faction) noexcept;

    virtual void AddVerts() noexcept;
    virtual void AddVertsForSelf() noexcept;

    Map* map = nullptr;
    Layer* layer = nullptr;
    Tile* tile = nullptr;
    EntityDefinition* def = nullptr;
    a2de::AnimatedSprite* sprite = nullptr;
    Inventory inventory{};
    a2de::Rgba color{a2de::Rgba::White};
    std::string name{"UNKNOWN ENTITY"};

    a2de::Event<const a2de::IntVector2&, const a2de::IntVector2&> OnMove;
    a2de::Event<Entity&, Entity&> OnFight;
    a2de::Event<DamageType, long, bool> OnDamage;
    a2de::Event<> OnMiss;
    a2de::Event<> OnDestroy;

protected:
    virtual void ResolveAttack(Entity& attacker, Entity& defender);
    Stats GetStatModifiers() const noexcept;
    const Stats& GetBaseStats() const noexcept;
    Stats& GetBaseStats() noexcept;

    a2de::Vector2 _screen_position{};
    a2de::IntVector2 _position{};
    Faction _faction{Faction::None};

private:
    void LoadFromXml(const a2de::XMLElement& elem);
    std::string ParseEntityDefinitionName(const a2de::XMLElement& xml_definition) const;
    
    void AddVertsForEquipment() const;
    void AddVertsForCapeEquipment() const;

    Stats stats{1,1,1};
    Stats stat_modifiers{};
    
};
