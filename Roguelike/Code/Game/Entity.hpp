#pragma once

#include "Engine/Math/IntVector2.hpp"

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

class Map;
class Layer;
class Tile;
class AnimatedSprite;
class EntityDefinition;

struct EntityType {
    EntityDefinition* definition{};
    std::string name{};
};

class Entity {
public:
    Entity() = default;
    Entity(const Entity& other) = default;
    Entity(Entity&& other) = default;
    Entity& operator=(const Entity& other) = default;
    Entity& operator=(Entity&& other) = default;
    virtual ~Entity() = default;

    Entity(EntityDefinition* definition) noexcept;
    Entity(const XMLElement& elem) noexcept;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;
    void EndFrame();

    void Move(const IntVector2& direction);
    void MoveWest();
    void MoveEast();
    void MoveNorth();
    void MoveSouth();

    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    void SetPosition(const IntVector2& position);

    Map* map = nullptr;
    Layer* layer = nullptr;
    Tile* tile = nullptr;
    AnimatedSprite* sprite = nullptr;
    EntityDefinition* def = nullptr;
    Rgba color{Rgba::White};
    std::string name{"UNKNOWN ENTITY"};
protected:
private:
    void LoadFromXml(const XMLElement& elem);
    std::string ParseEntityDefinitionName(const XMLElement& xml_definition);

    bool CanMoveDiagonallyToNeighbor(const IntVector2& direction);

    IntVector2 _position{};
};
