#include "Game/EquipmentDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include <sstream>

std::map<std::string, std::unique_ptr<EquipmentDefinition>> EquipmentDefinition::s_registry{};

void EquipmentDefinition::CreateEquipmentDefinition(Renderer& renderer, const XMLElement& elem) {
    auto new_def = std::make_unique<EquipmentDefinition>(renderer, elem);
    auto new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
}

void EquipmentDefinition::CreateEquipmentDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet) {
    auto new_def = std::make_unique<EquipmentDefinition>(renderer, elem, sheet);
    auto new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
}

void EquipmentDefinition::DestroyEquipmentDefinitions() {
    s_registry.clear();
}

EquipmentDefinition* const EquipmentDefinition::GetEquipmentDefinitionByName(const std::string& name) {
    auto found_iter = s_registry.find(name);
    if(found_iter == std::end(s_registry)) {
        return nullptr;
    }
    return found_iter->second.get();
}

EquipmentDefinition::EquipmentDefinition(Renderer& renderer, const XMLElement& elem)
    : _renderer(renderer)
{
    if(!LoadFromXml(elem)) {
        std::ostringstream ss;
        ss << "Could not load equipment definition " << elem.Name();
        ERROR_AND_DIE(ss.str().c_str());
    }
}

EquipmentDefinition::EquipmentDefinition(Renderer& renderer, const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet)
    : _renderer(renderer)
    , _sheet(sheet)
{
    if(!LoadFromXml(elem)) {
        std::ostringstream ss;
        ss << "Could not load equipment definition " << elem.Name();
        ERROR_AND_DIE(ss.str().c_str());
    }
}

const AnimatedSprite* EquipmentDefinition::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* EquipmentDefinition::GetSprite() {
    return const_cast<AnimatedSprite*>(static_cast<const EquipmentDefinition&>(*this).GetSprite());
}

bool EquipmentDefinition::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "equipmentDefinition", "", "name,index", "animation");
    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    _index = DataUtils::ParseXmlAttribute(elem, "index", _index);
    LoadAnimation(elem);
    return true;
}

void EquipmentDefinition::LoadAnimation(const XMLElement &elem) {
    if(auto* xml_animation = elem.FirstChildElement("animation")) {
        is_animated = true;
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, elem));
    } else {
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, _index));
    }
}

