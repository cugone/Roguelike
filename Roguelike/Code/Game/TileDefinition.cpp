#include "Game/TileDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"

#include <memory>

std::map<std::string, std::unique_ptr<TileDefinition>> TileDefinition::s_registry;

void TileDefinition::CreateTileDefinition(const XMLElement& elem, SpriteSheet* sheet) {
    auto new_def = std::make_unique<TileDefinition>(elem, sheet);
    s_registry.insert_or_assign(new_def->name, std::move(new_def));
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

Texture* TileDefinition::GetTexture() {
    return GetSheet()->GetTexture();
}

SpriteSheet* TileDefinition::GetSheet() {
    return _sheet;
}

TileDefinition::TileDefinition(const XMLElement& elem, SpriteSheet* sheet)
    : _sheet(sheet) {
    if(!LoadFromXml(elem)) {
        ERROR_AND_DIE("TileDefinition failed to load.\n");
    }
}

 bool TileDefinition::LoadFromXml(const XMLElement& elem) {

     DataUtils::ValidateXmlElement(elem, "tileDefinition", "glyph,opaque,solid", "name,index", "visible,allowDiagonalMovement");

     name = DataUtils::ParseXmlAttribute(elem, "name", name);
     index = DataUtils::ParseXmlAttribute(elem, "index", index);

     auto xml_glyph = elem.FirstChildElement("glyph");
     glyph = DataUtils::ParseXmlAttribute(*xml_glyph, "value", glyph);

     auto xml_opaque = elem.FirstChildElement("opaque");
     is_opaque = DataUtils::ParseXmlAttribute(*xml_opaque, "value", is_opaque);

     auto xml_solid = elem.FirstChildElement("solid");
     is_solid = DataUtils::ParseXmlAttribute(*xml_solid, "value", is_solid);

     if(auto xml_visible = elem.FirstChildElement("visible")) {
         is_visible = DataUtils::ParseXmlAttribute(*xml_visible, "value", is_visible);
     }

     if(auto xml_diag = elem.FirstChildElement("allowDiagonalMovement")) {
         allow_diagonal_movement = DataUtils::ParseXmlAttribute(*xml_diag, "value", allow_diagonal_movement);
     }

     return true;
 }
