#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/TimeUtils.hpp"
#include "Engine/Core/Vertex3D.hpp"

#include "Game/Stats.hpp"

#include <string>
#include <vector>

class EquipmentDefinition;
class Renderer;
class Map;
class Layer;
class Tile;
class AnimatedSprite;
class Entity;

struct EquipmentType {
    EquipmentDefinition* definition{};
    std::string name{};
    EquipmentType(std::string name = std::string{}, EquipmentDefinition* definition = nullptr)
        : definition(definition)
        , name(name)
    {
        /* DO NOTHING */
    }
};

class Equipment {
public:
    Equipment() = delete;
    explicit Equipment(Renderer& renderer, const XMLElement& elem) noexcept;
    Equipment(const Equipment& other) = default;
    Equipment(Equipment&& rother) = default;
    Equipment& operator=(const Equipment& other) = default;
    Equipment& operator=(Equipment&& rother) = default;
    ~Equipment() = default;

    void BeginFrame();
    void Update(TimeUtils::FPSeconds deltaSeconds);
    void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const;
    void EndFrame();

    //TODO: Change to Event "OnEquip"
    void ApplyStatModifer();
    //TODO: Change to Event "OnUnequip"
    void RemoveStatModifer();

    Map* map = nullptr;
    Layer* layer = nullptr;
    Tile* tile = nullptr;
    Entity* owner = nullptr;
    AnimatedSprite* sprite = nullptr;
    std::string name{"UNKNOWN EQUIPMENT"};
    EquipmentDefinition* def{};
    Stats stats{};
    
protected:
private:
    void LoadFromXml(const XMLElement& elem);
    std::string ParseEquipmentDefinitionName(const XMLElement& xml_definition);

    Renderer& _renderer;
};
