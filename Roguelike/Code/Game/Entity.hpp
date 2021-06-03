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
    , Player
    , Enemy
    , Neutral
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


    virtual bool IsVisible() const noexcept;
    virtual bool IsNotVisible() const noexcept;
    virtual bool IsInvisible() const noexcept;
    virtual bool IsSolid() const noexcept;
    virtual bool IsOpaque() const noexcept;

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

    static void AppendToMesh(const Entity* const entity) noexcept;

    virtual void CalculateLightValue() noexcept;
    uint32_t GetLightValue() const noexcept;
    void SetLightValue(uint32_t value) noexcept;

    Map* map = nullptr;
    Layer* layer = nullptr;
    Tile* tile = nullptr;
    AnimatedSprite* sprite = nullptr;
    Inventory inventory{};
    Rgba color{Rgba::White};
    std::string name{"UNKNOWN ENTITY"};

    Event<const IntVector2&, const IntVector2&> OnMove;
    Event<Entity&, Entity&> OnFight;
    Event<DamageType, long, bool> OnDamage;
    Event<> OnMiss;
    Event<> OnDestroy;

    void AddVertsForEquipment() const noexcept;
    void AddVertsForCapeEquipment() const noexcept;

protected:
    virtual void AppendToMesh() const noexcept;

    virtual void ResolveAttack(Entity& attacker, Entity& defender);
    Stats GetStatModifiers() const noexcept;
    const Stats& GetBaseStats() const noexcept;
    Stats& GetBaseStats() noexcept;

    Vector2 _screen_position{};
    IntVector2 _position{};
    Faction _faction{Faction::None};
    uint32_t _light_value{};
    uint32_t _self_illumination{};
private:
    void LoadFromXml(const XMLElement& elem);
    std::string ParseEntityDefinitionName(const XMLElement& xml_definition) const;

    Stats stats{1,1,1};
    Stats stat_modifiers{};

};
