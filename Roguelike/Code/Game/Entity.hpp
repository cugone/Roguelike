#pragma once

#include "Engine/Math/IntVector2.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Event.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Game/Inventory.hpp"
#include "Game/Stats.hpp"

class AnimatedSprite;
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
    Entity(const XMLElement& elem) noexcept;

    virtual void BeginFrame();
    virtual void Update(TimeUtils::FPSeconds deltaSeconds);
    virtual void EndFrame();

    static void Fight(Entity& attacker, Entity& defender);

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    virtual void SetPosition(const IntVector2& position);
    const IntVector2& GetPosition() const;
    const Vector2& GetScreenPosition() const;

    Stats GetStats() const;
    void AdjustBaseStats(Stats adjustments);
    void AdjustStatModifiers(Stats adjustments);

    Faction GetFaction() const noexcept;
    void SetFaction(const Faction& faction) noexcept;
    Faction JoinFaction(const Faction& faction) noexcept;
    Rgba GetFactionAsColor() const noexcept;

    virtual void AddVerts() noexcept;
    virtual void AddVertsForSelf() noexcept;

    Map* map = nullptr;
    Layer* layer = nullptr;
    Tile* tile = nullptr;
    EntityDefinition* def = nullptr;
    AnimatedSprite* sprite = nullptr;
    Inventory inventory{};
    Rgba color{Rgba::White};
    std::string name{"UNKNOWN ENTITY"};

    Event<const IntVector2&, const IntVector2&> OnMove;
    Event<Entity&, Entity&> OnFight;
    Event<DamageType, long, bool> OnDamage;
    Event<> OnMiss;
    Event<> OnDestroy;

protected:
    virtual void ResolveAttack(Entity& attacker, Entity& defender);
    Stats GetStatModifiers() const noexcept;
    const Stats& GetBaseStats() const noexcept;
    Stats& GetBaseStats() noexcept;

    Vector2 _screen_position{};
    IntVector2 _position{};
    Faction _faction{Faction::None};

private:
    void LoadFromXml(const XMLElement& elem);
    std::string ParseEntityDefinitionName(const XMLElement& xml_definition) const;
    
    void AddVertsForEquipment() const;
    void AddVertsForCapeEquipment() const;

    Stats stats{1,1,1};
    Stats stat_modifiers{};
    
};
