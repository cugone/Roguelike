#include "Game/EntityDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/EquipmentDefinition.hpp"
#include "Game/Map.hpp"

std::map<std::string, std::unique_ptr<EntityDefinition>> EntityDefinition::s_registry;

void EntityDefinition::CreateEntityDefinition(Renderer& renderer, const XMLElement& elem) {
    auto new_def = std::make_unique<EntityDefinition>(renderer, elem);
    auto new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
}

void EntityDefinition::CreateEntityDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet) {
    auto new_def = std::make_unique<EntityDefinition>(renderer, elem, sheet);
    auto new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
}

void EntityDefinition::DestroyEntityDefinitions() {
    s_registry.clear();
}

EntityDefinition* EntityDefinition::GetEntityDefinitionByName(const std::string& name) {
    auto found_iter = s_registry.find(name);
    if(found_iter == std::end(s_registry)) {
        return nullptr;
    }
    return found_iter->second.get();
}

void EntityDefinition::ClearEntityRegistry() {
    s_registry.clear();
}

std::vector<std::string> EntityDefinition::GetAllEntityDefinitionNames() {
    std::vector<std::string> result{};
    result.reserve(s_registry.size());
    for(const auto& e : s_registry) {
        result.push_back(e.first);
    }
    return result;
}

EntityDefinition::EntityDefinition(Renderer& renderer, const XMLElement& elem)
    : _renderer(renderer)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Entity Definition failed to load.");
    }
}

EntityDefinition::EntityDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet)
    : _renderer(renderer)
    , _sheet(sheet)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("Entity Definition failed to load.");
    }
}

const Stats& EntityDefinition::GetBaseStats() const noexcept {
    return _base_stats;
}

Stats& EntityDefinition::GetBaseStats() noexcept {
    return const_cast<Stats&>(static_cast<const EntityDefinition&>(*this).GetBaseStats());
}

void EntityDefinition::SetBaseStats(const Stats& newBaseStats) noexcept {
    _base_stats = newBaseStats;
}

const AnimatedSprite* EntityDefinition::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* EntityDefinition::GetSprite() {
    return const_cast<AnimatedSprite*>(static_cast<const EntityDefinition&>(*this).GetSprite());
}

bool EntityDefinition::HasAttachPoint(const AttachPoint& attachpoint) {
    return _valid_offsets[static_cast<std::size_t>(attachpoint)];
}

Vector2 EntityDefinition::GetAttachPoint(const AttachPoint& attachpoint) {
    if(HasAttachPoint(attachpoint)) {
        return attach_point_offsets[static_cast<std::size_t>(attachpoint)];
    }
    return {};
}

bool EntityDefinition::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "entityDefinition", "", "name,index", "animation,attachPoints,equipment,stats");

    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    _index = DataUtils::ParseXmlAttribute(elem, "index", IntVector2::ZERO);
    LoadStats(elem);
    LoadAnimation(elem);
    LoadAttachPoints(elem);
    LoadEquipment(elem);
    return true;
}

void EntityDefinition::LoadStats(const XMLElement& elem) {
    if(auto* xml_stats = elem.FirstChildElement("stats")) {
        _base_stats = Stats(*xml_stats);
    }
}

void EntityDefinition::LoadEquipment(const XMLElement& elem) {
    if(auto* xml_equipment = elem.FirstChildElement("equipment")) {
        DataUtils::ValidateXmlElement(*xml_equipment, "equipment", "", "", "hair,head,body,larm,rarm,legs,feet");
        equipments.resize(static_cast<std::size_t>(EquipSlot::Max));
        if(auto* xml_equipment_hair = xml_equipment->FirstChildElement("hair")) {
            DataUtils::ValidateXmlElement(*xml_equipment_hair, "hair", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_hair, "item", equipment_name);
            auto idx = static_cast<std::size_t>(EquipSlot::Hair);
            auto def = Map::GetEquipmentTypesByName(equipment_name);
            if(!def.empty()) {
                _valid_offsets.set(idx);
                auto f = std::begin(def);
                equipments[idx] = (*f)->definition;
            }
        }
        if(auto* xml_equipment_head = xml_equipment->FirstChildElement("head")) {
            DataUtils::ValidateXmlElement(*xml_equipment_head, "head", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_head, "item", equipment_name);
            auto idx = static_cast<std::size_t>(EquipSlot::Head);
            auto def = Map::GetEquipmentTypesByName(equipment_name);
            if(!def.empty()) {
                _valid_offsets.set(idx);
                auto f = std::begin(def);
                equipments[idx] = (*f)->definition;
            }
        }
        if(auto* xml_equipment_body = xml_equipment->FirstChildElement("body")) {
            DataUtils::ValidateXmlElement(*xml_equipment_body, "body", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_body, "item", equipment_name);
            auto idx = static_cast<std::size_t>(EquipSlot::Body);
            auto def = Map::GetEquipmentTypesByName(equipment_name);
            if(!def.empty()) {
                _valid_offsets.set(idx);
                auto f = std::begin(def);
                equipments[idx] = (*f)->definition;
            }
        }
        if(auto* xml_equipment_larm = xml_equipment->FirstChildElement("larm")) {
            DataUtils::ValidateXmlElement(*xml_equipment_larm, "larm", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_larm, "item", equipment_name);
            auto idx = static_cast<std::size_t>(EquipSlot::LeftArm);
            auto def = Map::GetEquipmentTypesByName(equipment_name);
            if(!def.empty()) {
                _valid_offsets.set(idx);
                auto f = std::begin(def);
                equipments[idx] = (*f)->definition;
            }
        }
        if(auto* xml_equipment_rarm = xml_equipment->FirstChildElement("rarm")) {
            DataUtils::ValidateXmlElement(*xml_equipment_rarm, "rarm", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_rarm, "item", equipment_name);
            auto idx = static_cast<std::size_t>(EquipSlot::RightArm);
            auto def = Map::GetEquipmentTypesByName(equipment_name);
            if(!def.empty()) {
                _valid_offsets.set(idx);
                auto f = std::begin(def);
                equipments[idx] = (*f)->definition;
            }
        }
        if(auto* xml_equipment_legs = xml_equipment->FirstChildElement("legs")) {
            DataUtils::ValidateXmlElement(*xml_equipment_legs, "legs", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_legs, "item", equipment_name);
            auto idx = static_cast<std::size_t>(EquipSlot::Legs);
            auto def = Map::GetEquipmentTypesByName(equipment_name);
            if(!def.empty()) {
                _valid_offsets.set(idx);
                auto f = std::begin(def);
                equipments[idx] = (*f)->definition;
            }
        }
        if(auto* xml_equipment_feet = xml_equipment->FirstChildElement("feet")) {
            DataUtils::ValidateXmlElement(*xml_equipment_feet, "feet", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_feet, "item", equipment_name);
            auto idx = static_cast<std::size_t>(EquipSlot::Feet);
            auto def = Map::GetEquipmentTypesByName(equipment_name);
            if(!def.empty()) {
                _valid_offsets.set(idx);
                auto f = std::begin(def);
                equipments[idx] = (*f)->definition;
            }
        }
    }
}

void EntityDefinition::LoadAttachPoints(const XMLElement &elem) {
    if(auto* xml_attachPoints = elem.FirstChildElement("attachPoints")) {
        DataUtils::ValidateXmlElement(*xml_attachPoints, "attachPoints", "", "", "hair,head,body,larm,rarm,legs,feet");
        attach_point_offsets.resize(static_cast<std::size_t>(AttachPoint::Max));
        if(auto* xml_attachPoint_hair = xml_attachPoints->FirstChildElement("hair")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_hair, "hair", "", "offset");
            attach_point_offsets[0] = DataUtils::ParseXmlAttribute(*xml_attachPoint_hair, "offset", attach_point_offsets[0]);
        }
        if(auto* xml_attachPoint_head = xml_attachPoints->FirstChildElement("head")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_head, "head", "", "offset");
            attach_point_offsets[1] = DataUtils::ParseXmlAttribute(*xml_attachPoint_head, "offset", attach_point_offsets[1]);
        }
        if(auto* xml_attachPoint_body = xml_attachPoints->FirstChildElement("body")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_body, "body", "", "offset");
            attach_point_offsets[2] = DataUtils::ParseXmlAttribute(*xml_attachPoint_body, "offset", attach_point_offsets[2]);
        }
        if(auto* xml_attachPoint_larm = xml_attachPoints->FirstChildElement("larm")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_larm, "larm", "", "offset");
            attach_point_offsets[3] = DataUtils::ParseXmlAttribute(*xml_attachPoint_larm, "offset", attach_point_offsets[3]);
        }
        if(auto* xml_attachPoint_rarm = xml_attachPoints->FirstChildElement("rarm")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_rarm, "rarm", "", "offset");
            attach_point_offsets[4] = DataUtils::ParseXmlAttribute(*xml_attachPoint_rarm, "offset", attach_point_offsets[4]);
        }
        if(auto* xml_attachPoint_legs = xml_attachPoints->FirstChildElement("legs")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_legs, "legs", "", "offset");
            attach_point_offsets[5] = DataUtils::ParseXmlAttribute(*xml_attachPoint_legs, "offset", attach_point_offsets[5]);
        }
        if(auto* xml_attachPoint_feet = xml_attachPoints->FirstChildElement("feet")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_feet, "feet", "", "offset");
            attach_point_offsets[7] = DataUtils::ParseXmlAttribute(*xml_attachPoint_feet, "offset", attach_point_offsets[7]);
        }
    }
}

void EntityDefinition::LoadAnimation(const XMLElement &elem) {
    if(auto* xml_animation = elem.FirstChildElement("animation")) {
        is_animated = true;
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, elem));
    } else {
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, _index));
    }
}

EquipSlot EquipSlotFromString(std::string str) {
    str = StringUtils::ToLowerCase(str);
    if(str == "hair") {
        return EquipSlot::Hair;
    } else if(str == "head") {
        return EquipSlot::Head;
    } else if(str == "body") {
        return EquipSlot::Body;
    } else if(str == "larm") {
        return EquipSlot::LeftArm;
    } else if(str == "rarm") {
        return EquipSlot::RightArm;
    } else if(str == "legs") {
        return EquipSlot::Legs;
    } else if(str == "feet") {
        return EquipSlot::Feet;
    } else {
        return EquipSlot::None;
    }
}

std::string EquipSlotToString(const EquipSlot& slot) {
    switch(slot) {
    case EquipSlot::Hair: return "hair";
    case EquipSlot::Head: return "head";
    case EquipSlot::Body: return "body";
    case EquipSlot::LeftArm: return "larm";
    case EquipSlot::RightArm: return "rarm";
    case EquipSlot::Legs: return "legs";
    case EquipSlot::Feet: return "feet";
    default: return "none";
    }
}
