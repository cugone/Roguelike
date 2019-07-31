#include "Game/TileDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Game/GameCommon.hpp"

#include <memory>

std::map<std::string, std::unique_ptr<TileDefinition>> TileDefinition::s_registry{};

void TileDefinition::CreateTileDefinition(Renderer* renderer, const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet) {
    auto new_def = std::make_unique<TileDefinition>(renderer, elem, sheet);
    auto new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
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

const Texture* TileDefinition::GetTexture() const {
    return GetSheet()->GetTexture();
}

Texture* TileDefinition::GetTexture() {
    return const_cast<Texture*>(static_cast<const TileDefinition&>(*this).GetTexture());
}

const SpriteSheet* TileDefinition::GetSheet() const {
    if(!_sheet.expired()) {
        return _sheet.lock().get();
    }
    return nullptr;
}

SpriteSheet* TileDefinition::GetSheet() {
    return const_cast<SpriteSheet*>(static_cast<const TileDefinition&>(*this).GetSheet());
}

const AnimatedSprite* TileDefinition::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* TileDefinition::GetSprite() {
    return const_cast<AnimatedSprite*>(static_cast<const TileDefinition&>(*this).GetSprite());
}

IntVector2 TileDefinition::GetIndexCoords() const {
    return _index;
}

int TileDefinition::GetIndex() const {
    if(auto* sheet = GetSheet()) {
        return (_index.x + _random_index_offset) + _index.y * sheet->GetLayout().x;
    }
    return -1;
}

TileDefinition::TileDefinition(Renderer* renderer, const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet)
    : _renderer(renderer)
    , _sheet(sheet)
{
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("TileDefinition failed to load.\n");
    }
}

 bool TileDefinition::LoadFromXml(const XMLElement& elem) {

     DataUtils::ValidateXmlElement(elem, "tileDefinition", "glyph", "name,index", "opaque,solid,visible,invisible,allowDiagonalMovement,animation,offset");

     name = DataUtils::ParseXmlAttribute(elem, "name", name);
     if(auto xml_randomOffset = elem.FirstChildElement("offset")) {
         _random_index_offset = DataUtils::ParseXmlAttribute(*xml_randomOffset, "value", _random_index_offset);
     }
     _index = DataUtils::ParseXmlAttribute(elem, "index", _index);
     AddOffsetToIndex(_random_index_offset);

     auto xml_glyph = elem.FirstChildElement("glyph");
     glyph = DataUtils::ParseXmlAttribute(*xml_glyph, "value", glyph);

     if(auto xml_opaque = elem.FirstChildElement("opaque")) {
         is_opaque = true;
         is_opaque = DataUtils::ParseXmlAttribute(*xml_opaque, "value", is_opaque);
     }

     if(auto xml_solid = elem.FirstChildElement("solid")) {
         is_solid = true;
         is_solid = DataUtils::ParseXmlAttribute(*xml_solid, "value", is_solid);
     }

     if(auto xml_visible = elem.FirstChildElement("visible")) {
         is_visible = true;
         is_visible = DataUtils::ParseXmlAttribute(*xml_visible, "value", is_visible);
     }
     if(auto xml_invisible = elem.FirstChildElement("invisible")) {
         is_visible = false;
         is_visible = DataUtils::ParseXmlAttribute(*xml_invisible, "value", is_visible);
     }

     if(auto xml_diag = elem.FirstChildElement("allowDiagonalMovement")) {
         allow_diagonal_movement = true;
         allow_diagonal_movement = DataUtils::ParseXmlAttribute(*xml_diag, "value", allow_diagonal_movement);
     }

     if(auto xml_animation = elem.FirstChildElement("animation")) {
         is_animated = true;
         _sprite = std::move(_renderer->CreateAnimatedSprite(_sheet, elem));
     } else {
         _sprite = std::move(_renderer->CreateAnimatedSprite(_sheet, _index));
     }
     return true;
 }

 void TileDefinition::SetIndex(const IntVector2& indexCoords) {
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
     SetIndex(IntVector2{ x, y });
 }

 void TileDefinition::AddOffsetToIndex(int offset) {
     const auto x = _index.x + offset;
     const auto y = _index.y;
     SetIndex(x, y);
 }
