#include "Game/CursorDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/GameCommon.hpp"

#include <memory>

std::map<std::string, std::unique_ptr<CursorDefinition>> CursorDefinition::s_registry;

const std::map<std::string, std::unique_ptr<CursorDefinition>>& CursorDefinition::GetLoadedDefinitions() {
    return s_registry;
}

void CursorDefinition::CreateCursorDefinition(Renderer& renderer, const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet) {
    auto new_def = std::make_unique<CursorDefinition>(renderer, elem, sheet);
    auto new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
}

void CursorDefinition::DestroyCursorDefinitions() {
    s_registry.clear();
}

CursorDefinition* CursorDefinition::GetCursorDefinitionByName(const std::string& name) {
    auto found_iter = s_registry.find(name);
    if(found_iter != std::end(s_registry)) {
        return found_iter->second.get();
    }
    return nullptr;
}

void CursorDefinition::ClearCursorRegistry() {
    s_registry.clear();
}

const Texture* CursorDefinition::GetTexture() const {
    return GetSheet()->GetTexture();
}

Texture* CursorDefinition::GetTexture() {
    return const_cast<Texture*>(static_cast<const CursorDefinition&>(*this).GetTexture());
}

const SpriteSheet* CursorDefinition::GetSheet() const {
    if(!_sheet.expired()) {
        return _sheet.lock().get();
    }
    return nullptr;
}

SpriteSheet* CursorDefinition::GetSheet() {
    return const_cast<SpriteSheet*>(static_cast<const CursorDefinition&>(*this).GetSheet());
}

const AnimatedSprite* CursorDefinition::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* CursorDefinition::GetSprite() {
    return _sprite.get();
}

IntVector2 CursorDefinition::GetIndexCoords() const {
    return _index;
}

int CursorDefinition::GetIndex() const {
    if(auto* sheet = GetSheet()) {
        return (_index.x) + _index.y * sheet->GetLayout().x;
    }
    return -1;
}

CursorDefinition::CursorDefinition(Renderer& renderer, const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet)
    : _renderer(renderer)
    , _sheet(sheet)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("CursorDefinition failed to load.\n");
    }
}

bool CursorDefinition::LoadFromXml(const XMLElement& elem) {

    DataUtils::ValidateXmlElement(elem, "cursor", "", "name,index", "animation");

    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    _index = DataUtils::ParseXmlAttribute(elem, "index", _index);

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
    SetIndex(IntVector2{x, y});
}

void CursorDefinition::SetIndex(const IntVector2& indexCoords) {
    _index = indexCoords;
}

