#include "Game/EntityDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

std::map<std::string, std::unique_ptr<EntityDefinition>> EntityDefinition::s_registry;

void EntityDefinition::CreateEntityDefinition(Renderer& renderer, const XMLElement& elem) {
    auto new_def = std::make_unique<EntityDefinition>(renderer, elem);
    s_registry.insert_or_assign(new_def->name, std::move(new_def));
}

void EntityDefinition::CreateEntityDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet) {
    auto new_def = std::make_unique<EntityDefinition>(renderer, elem, sheet);
    s_registry.insert_or_assign(new_def->name, std::move(new_def));
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

const AnimatedSprite* EntityDefinition::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* EntityDefinition::GetSprite() {
    return const_cast<AnimatedSprite*>(static_cast<const EntityDefinition&>(*this).GetSprite());
}

bool EntityDefinition::HasAttachPoint(const AttachPoint& attachpoint) {
    return _valid_offsets[static_cast<std::size_t>(attachpoint)];
}

bool EntityDefinition::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "entityDefinition", "", "name,index", "animation,attachPoints,equipment");

    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    _index = DataUtils::ParseXmlAttribute(elem, "index", IntVector2::ZERO);
    LoadAnimation(elem);
    LoadAttachPoints(elem);
    LoadEquipment(elem);
    return true;
}

void EntityDefinition::LoadEquipment(const XMLElement& elem) {
    if(auto* xml_equipment = elem.FirstChildElement("equipment")) {
        DataUtils::ValidateXmlElement(*xml_equipment, "equipment", "", "", "hair,head,body,larm,rarm,lleg,rleg,feet");
        if(auto* xml_equipment_hair = xml_equipment->FirstChildElement("hair")) {
            DataUtils::ValidateXmlElement(*xml_equipment_hair, "hair", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_hair, "item", equipment_name);
        }
        if(auto* xml_equipment_head = xml_equipment->FirstChildElement("head")) {
            DataUtils::ValidateXmlElement(*xml_equipment_head, "head", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_head, "item", equipment_name);
        }
        if(auto* xml_equipment_body = xml_equipment->FirstChildElement("body")) {
            DataUtils::ValidateXmlElement(*xml_equipment_body, "body", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_body, "item", equipment_name);
        }
        if(auto* xml_equipment_larm = xml_equipment->FirstChildElement("larm")) {
            DataUtils::ValidateXmlElement(*xml_equipment_larm, "larm", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_larm, "item", equipment_name);
        }
        if(auto* xml_equipment_rarm = xml_equipment->FirstChildElement("rarm")) {
            DataUtils::ValidateXmlElement(*xml_equipment_rarm, "rarm", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_rarm, "item", equipment_name);
        }
        if(auto* xml_equipment_lleg = xml_equipment->FirstChildElement("lleg")) {
            DataUtils::ValidateXmlElement(*xml_equipment_lleg, "lleg", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_lleg, "item", equipment_name);
        }
        if(auto* xml_equipment_rleg = xml_equipment->FirstChildElement("rleg")) {
            DataUtils::ValidateXmlElement(*xml_equipment_rleg, "rleg", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_rleg, "item", equipment_name);
        }
        if(auto* xml_equipment_feet = xml_equipment->FirstChildElement("feet")) {
            DataUtils::ValidateXmlElement(*xml_equipment_feet, "feet", "", "item");
            std::string equipment_name{};
            equipment_name = DataUtils::ParseXmlAttribute(*xml_equipment_feet, "item", equipment_name);
        }
    }
}

void EntityDefinition::LoadAttachPoints(const XMLElement &elem) {
    if(auto* xml_attachPoints = elem.FirstChildElement("attachPoints")) {
        DataUtils::ValidateXmlElement(*xml_attachPoints, "attachPoints", "", "", "hair,head,body,larm,rarm,lleg,rleg,feet");
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
        if(auto* xml_attachPoint_lleg = xml_attachPoints->FirstChildElement("lleg")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_lleg, "lleg", "", "offset");
            attach_point_offsets[5] = DataUtils::ParseXmlAttribute(*xml_attachPoint_lleg, "offset", attach_point_offsets[5]);
        }
        if(auto* xml_attachPoint_rleg = xml_attachPoints->FirstChildElement("rleg")) {
            DataUtils::ValidateXmlElement(*xml_attachPoint_rleg, "rleg", "", "offset");
            attach_point_offsets[6] = DataUtils::ParseXmlAttribute(*xml_attachPoint_rleg, "offset", attach_point_offsets[6]);
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
    } else if(str == "lleg") {
        return EquipSlot::LeftLeg;
    } else if(str == "rleg") {
        return EquipSlot::RightLeg;
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
    case EquipSlot::LeftLeg: return "lleg";
    case EquipSlot::RightLeg: return "rleg";
    case EquipSlot::Feet: return "feet";
    default: return "none";
    }
}
