#include "Game/EntityDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

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

bool EntityDefinition::LoadFromXml(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "entityDefinition", "", "name,index", "animation");

    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    _index = DataUtils::ParseXmlAttribute(elem, "index", IntVector2::ZERO);
    if(auto* xml_animation = elem.FirstChildElement("animation")) {
        is_animated = true;
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, elem));
    } else {
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, _index));
    }
    return true;
}

