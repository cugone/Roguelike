#include "Game/TileDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"

#include <memory>

std::map<std::string, std::unique_ptr<TileDefinition>> TileDefinition::s_registry;

void TileDefinition::CreateTileDefinition(const XMLElement& elem, SpriteSheet* sheet) {
    auto new_def = std::make_unique<TileDefinition>(elem, sheet);
    s_registry.insert_or_assign(new_def->_name, std::move(new_def));
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
        if(tile.second->_glyph == glyph) {
            return tile.second.get();
        }
    }
    return nullptr;
}

void TileDefinition::ClearTileRegistry() {
    s_registry.clear();
}

const Texture* TileDefinition::GetTexture() const {
    return &(GetSheet()->GetTexture());
}

const SpriteSheet* TileDefinition::GetSheet() const {
    return _sheet;
}

TileDefinition::TileDefinition(const XMLElement& elem, SpriteSheet* sheet)
    : _sheet(sheet) {
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("TileDefinition failed to load.\n");
    }
}

 bool TileDefinition::LoadFromXml(const XMLElement& elem) {

     DataUtils::ValidateXmlElement(elem, "tileDefinition", "glyph,opaque,solid", "name,index", "allowDiagonalMovement");

     _name = DataUtils::ParseXmlAttribute(elem, "name", _name);
     _index = DataUtils::ParseXmlAttribute(elem, "index", _index);

     auto xml_glyph = elem.FirstChildElement("glyph");
     _glyph = DataUtils::ParseXmlAttribute(*xml_glyph, "value", _glyph);

     auto xml_opaque = elem.FirstChildElement("opaque");
     _is_opaque = DataUtils::ParseXmlAttribute(*xml_opaque, "value", _is_opaque);

     auto xml_solid = elem.FirstChildElement("solid");
     _is_solid = DataUtils::ParseXmlAttribute(*xml_solid, "value", _is_solid);

     if(auto xml_diag = elem.FirstChildElement("allowDiagonalMovement")) {
         _allow_diagonal_movement = DataUtils::ParseXmlAttribute(*xml_diag, "value", _allow_diagonal_movement);
     }

     return true;
 }
