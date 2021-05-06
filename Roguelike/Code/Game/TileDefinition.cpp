#include "Game/TileDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/GameCommon.hpp"

#include <memory>

std::map<std::string, std::unique_ptr<TileDefinition>> TileDefinition::s_registry{};

TileDefinition* TileDefinition::CreateTileDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet) {
    auto new_def = std::make_unique<TileDefinition>(renderer, elem, sheet);
    auto* new_def_ptr = new_def.get();
    std::string new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
    return new_def_ptr;
}

void TileDefinition::DestroyTileDefinitions() {
    s_registry.clear();
}

TileDefinition* TileDefinition::GetTileDefinitionByName(const std::string& name) {
    auto found_iter = s_registry.find(name);
    if(found_iter != std::end(s_registry)) {
        return found_iter->second.get();
    }
    return nullptr;
}

TileDefinition* TileDefinition::GetTileDefinitionByGlyph(char glyph) {
    for(const auto& tile : s_registry) {
        if(tile.second->glyph == glyph) {
            return tile.second.get();
        }
    }
    return nullptr;
}

void TileDefinition::ClearTileRegistry() {
    s_registry.clear();
}

const a2de::Texture* TileDefinition::GetTexture() const {
    return GetSheet()->GetTexture();
}

a2de::Texture* TileDefinition::GetTexture() {
    return const_cast<a2de::Texture*>(static_cast<const TileDefinition&>(*this).GetTexture());
}

const a2de::SpriteSheet* TileDefinition::GetSheet() const {
    if(!_sheet.expired()) {
        return _sheet.lock().get();
    }
    return nullptr;
}

a2de::SpriteSheet* TileDefinition::GetSheet() {
    return const_cast<a2de::SpriteSheet*>(static_cast<const TileDefinition&>(*this).GetSheet());
}

const a2de::AnimatedSprite* TileDefinition::GetSprite() const {
    return _sprite.get();
}

a2de::AnimatedSprite* TileDefinition::GetSprite() {
    return _sprite.get();
}

a2de::IntVector2 TileDefinition::GetIndexCoords() const {
    return _index;
}

int TileDefinition::GetIndex() const {
    if(auto* sheet = GetSheet()) {
        return (_index.x + _random_index_offset) + _index.y * sheet->GetLayout().x;
    }
    return -1;
}

TileDefinition::TileDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet)
    : _renderer(renderer)
    , _sheet(sheet)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("TileDefinition failed to load.\n");
    }
}

//TODO: Fix extraneous leading space
 bool TileDefinition::LoadFromXml(const a2de::XMLElement& elem) {

     a2de::DataUtils::ValidateXmlElement(elem, "tileDefinition", "glyph", "name,index", "opaque,solid,visible,invisible,allowDiagonalMovement,animation,offset");

     name = a2de::DataUtils::ParseXmlAttribute(elem, "name", name);
     if(auto xml_randomOffset = elem.FirstChildElement("offset")) {
         _random_index_offset = a2de::DataUtils::ParseXmlAttribute(*xml_randomOffset, "value", _random_index_offset);
     }
     _index = a2de::DataUtils::ParseXmlAttribute(elem, "index", _index);
     AddOffsetToIndex(_random_index_offset);

     auto xml_glyph = elem.FirstChildElement("glyph");
     glyph = a2de::DataUtils::ParseXmlAttribute(*xml_glyph, "value", glyph);

     if(auto xml_opaque = elem.FirstChildElement("opaque")) {
         is_opaque = true;
         is_opaque = a2de::DataUtils::ParseXmlAttribute(*xml_opaque, "value", is_opaque);
     }

     if(auto xml_solid = elem.FirstChildElement("solid")) {
         is_solid = true;
         is_solid = a2de::DataUtils::ParseXmlAttribute(*xml_solid, "value", is_solid);
     }

     if(auto xml_visible = elem.FirstChildElement("visible")) {
         is_visible = true;
         is_visible = a2de::DataUtils::ParseXmlAttribute(*xml_visible, "value", is_visible);
     }
     if(auto xml_invisible = elem.FirstChildElement("invisible")) {
         is_visible = false;
         is_visible = a2de::DataUtils::ParseXmlAttribute(*xml_invisible, "value", is_visible);
     }

     if(auto xml_diag = elem.FirstChildElement("allowDiagonalMovement")) {
         allow_diagonal_movement = true;
         allow_diagonal_movement = a2de::DataUtils::ParseXmlAttribute(*xml_diag, "value", allow_diagonal_movement);
     }

     if(auto xml_animation = elem.FirstChildElement("animation")) {
         is_animated = true;
         _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, *xml_animation));
     } else {
         _sprite = std::move(_renderer.CreateAnimatedSprite(_sheet, _index));
     }
     return true;
 }

 void TileDefinition::SetIndex(const a2de::IntVector2& indexCoords) {
     _index = indexCoords;
 }

 void TileDefinition::SetIndex(int index) {
     if(auto sheet = GetSheet()) {
         const auto& layout = sheet->GetLayout();
         const auto x = index % layout.x;
         const auto y = index / layout.x;
         SetIndex(x, y);
     }
 }

 void TileDefinition::SetIndex(int x, int y) {
     SetIndex(a2de::IntVector2{ x, y });
 }

 void TileDefinition::AddOffsetToIndex(int offset) {
     const auto x = _index.x + offset;
     const auto y = _index.y;
     SetIndex(x, y);
 }
