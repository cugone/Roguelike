#pragma once

#include "Engine/Core/DataUtils.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/AnimatedSprite.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class Renderer;

class EquipmentDefinition {
public:

    static void CreateEquipmentDefinition(Renderer& renderer, const XMLElement& elem);
    static void CreateEquipmentDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet);
    static void DestroyEquipmentDefinitions();

    static EquipmentDefinition* const GetEquipmentDefinitionByName(const std::string& name);
    EquipmentDefinition() = delete;
    EquipmentDefinition(const EquipmentDefinition& other) = default;
    EquipmentDefinition(EquipmentDefinition&& r_other) = default;
    EquipmentDefinition& operator=(const EquipmentDefinition& other) = default;
    EquipmentDefinition& operator=(EquipmentDefinition&& r_other) = default;
    ~EquipmentDefinition() = default;

    EquipmentDefinition(Renderer& renderer, const XMLElement& elem);
    EquipmentDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet);


    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();

    std::string name{};
    bool is_animated = false;
protected:
private:

    bool LoadFromXml(const XMLElement& elem);
    void LoadAnimation(const XMLElement &elem);

    Renderer& _renderer;
    std::shared_ptr<SpriteSheet> _sheet{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    IntVector2 _index{};
    static std::map<std::string, std::unique_ptr<EquipmentDefinition>> s_registry;

};

