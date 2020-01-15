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
    virtual void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;
    virtual void EndFrame();

    void UpdateAI(TimeUtils::FPSeconds deltaSeconds);

    static long double Fight(Entity& attacker, Entity& defender);

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    virtual void SetPosition(const IntVector2& position);
    const IntVector2& GetPosition() const;
    const Vector2& GetScreenPosition() const;
    void SetScreenPosition(const Vector2& screenPosition);

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

    Event<const IntVector2&, const IntVector2&> OnMove;
    Event<Entity&, Entity&, long double> OnFight;
    Event<DamageType, long double> OnDamage;
    Event<> OnDestroy;
protected:
    Stats GetStatModifiers() const noexcept;
    const Stats& GetBaseStats() const noexcept;
    Stats& GetBaseStats() noexcept;

    Vector2 _screen_position{};
    IntVector2 _position{};
private:
    void LoadFromXml(const XMLElement& elem);
    std::string ParseEntityDefinitionName(const XMLElement& xml_definition) const;

    void AddVertsForEquipment(const IntVector2& entity_position, std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;

    void ApplyDamage(DamageType type, long double amount);

    Stats stats{1,1,1};
    Stats stat_modifiers{};
    
};
