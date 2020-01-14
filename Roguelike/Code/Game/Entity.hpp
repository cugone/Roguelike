#pragma once

#include "Engine/Math/IntVector2.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Event.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Game/Inventory.hpp"
#include "Game/Stats.hpp"

class Map;
class Layer;
class Tile;
class AnimatedSprite;
class EntityDefinition;

class Entity {
public:
    Entity() = default;
    Entity(const Entity& other) = default;
    Entity(Entity&& other) = default;
    Entity& operator=(const Entity& other) = default;
    Entity& operator=(Entity&& other) = default;
    virtual ~Entity() = 0;

    Entity(EntityDefinition* definition) noexcept;
    Entity(const XMLElement& elem) noexcept;

    virtual void BeginFrame();
    virtual void Update(TimeUtils::FPSeconds deltaSeconds);
    virtual void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;
    virtual void EndFrame();

    void UpdateAI(TimeUtils::FPSeconds deltaSeconds);

    static long double Fight(Entity& attacker, Entity& defender);

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    void SetPosition(const IntVector2& position);
    const IntVector2& GetPosition() const;

    Stats GetStats() const;
    void AdjustBaseStats(Stats adjustments);
    void AdjustStatModifiers(Stats adjustments);

    Map* map = nullptr;
    Layer* layer = nullptr;
    Tile* tile = nullptr;
    EntityDefinition* def = nullptr;
    AnimatedSprite* sprite = nullptr;
    Inventory inventory{};
    Rgba color{Rgba::White};
    std::string name{"UNKNOWN ENTITY"};

    static Event<const IntVector2&, const IntVector2&> OnMove;
    static Event<Entity&, Entity&, long double> OnFight;
    static Event<DamageType, long double> OnDamage;
    static Event<> OnDestroy;
protected:
    Stats GetStatModifiers() const noexcept;
    const Stats& GetBaseStats() const noexcept;
    Stats& GetBaseStats() noexcept;
private:
    void LoadFromXml(const XMLElement& elem);
    std::string ParseEntityDefinitionName(const XMLElement& xml_definition) const;

    void AddVertsForEquipment(const IntVector2& entity_position, std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;

    void ApplyDamage(DamageType type, long double amount);

    Stats stats{};
    Stats stat_modifiers{};
    IntVector2 _position{};

};
