#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Renderer/AnimatedSprite.hpp"

#include <map>
#include <memory>

class AnimatedSprite;
class SpriteSheet;
class Renderer;

class EntityDefinition {
public:

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
    IntVector2 index{};
    bool is_invisible = false;
    bool is_animated = false;

    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();

protected:
private:
    bool LoadFromXml(const XMLElement& elem);

    static std::map<std::string, std::unique_ptr<EntityDefinition>> s_registry;
    Renderer& _renderer;
    std::shared_ptr<SpriteSheet> _sheet{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    IntVector2 _index{};
};
