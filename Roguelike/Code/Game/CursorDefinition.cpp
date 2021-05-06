#include "Game/CursorDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/GameCommon.hpp"

#include <memory>

std::vector<std::unique_ptr<CursorDefinition>> CursorDefinition::s_registry;

const std::vector<std::unique_ptr<CursorDefinition>>& CursorDefinition::GetLoadedDefinitions() {
    return s_registry;
}

void CursorDefinition::CreateCursorDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet) {
    s_registry.emplace_back(std::move(std::make_unique<CursorDefinition>(renderer, elem, sheet)));
}

void CursorDefinition::DestroyCursorDefinitions() {
    s_registry.clear();
}

CursorDefinition* CursorDefinition::GetCursorDefinitionByName(const std::string& name) {
    auto found_iter = std::find_if(std::begin(s_registry), std::end(s_registry), [name](auto&& c) { return c.get()->name == name; });
    if(found_iter != std::end(s_registry)) {
        return found_iter->get();
    }
    return nullptr;
}

void CursorDefinition::ClearCursorRegistry() {
    s_registry.clear();
}

const a2de::Texture* CursorDefinition::GetTexture() const {
    return GetSheet()->GetTexture();
}

a2de::Texture* CursorDefinition::GetTexture() {
    return const_cast<a2de::Texture*>(static_cast<const CursorDefinition&>(*this).GetTexture());
}

const a2de::SpriteSheet* CursorDefinition::GetSheet() const {
    if(!_sheet.expired()) {
        return _sheet.lock().get();
    }
    return nullptr;
}

a2de::SpriteSheet* CursorDefinition::GetSheet() {
    return const_cast<a2de::SpriteSheet*>(static_cast<const CursorDefinition&>(*this).GetSheet());
}

const a2de::AnimatedSprite* CursorDefinition::GetSprite() const {
    return _sprite.get();
}

a2de::AnimatedSprite* CursorDefinition::GetSprite() {
    return _sprite.get();
}

a2de::IntVector2 CursorDefinition::GetIndexCoords() const {
    return _index;
}

int CursorDefinition::GetIndex() const {
    if(auto* sheet = GetSheet()) {
        return (_index.x) + _index.y * sheet->GetLayout().x;
    }
    return -1;
}

CursorDefinition::CursorDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet)
    : _renderer(renderer)
    , _sheet(sheet)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("CursorDefinition failed to load.\n");
    }
}

bool CursorDefinition::LoadFromXml(const a2de::XMLElement& elem) {

    a2de::DataUtils::ValidateXmlElement(elem, "cursor", "", "name,index", "animation");

    name = a2de::DataUtils::ParseXmlAttribute(elem, "name", name);
    _index = a2de::DataUtils::ParseXmlAttribute(elem, "index", _index);

    if(auto xml_animation = elem.FirstChildElement("animation")) {
        is_animated = true;
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, *xml_animation));
    } else {
        _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, _index));
    }
    return true;
}

void CursorDefinition::SetIndex(int index) {
    if(auto sheet = GetSheet()) {
        const auto& layout = sheet->GetLayout();
        const auto x = index % layout.x;
        const auto y = index / layout.x;
        SetIndex(x, y);
    }
}

void CursorDefinition::SetIndex(int x, int y) {
    SetIndex(a2de::IntVector2{x, y});
}

void CursorDefinition::SetIndex(const a2de::IntVector2& indexCoords) {
    _index = indexCoords;
}

