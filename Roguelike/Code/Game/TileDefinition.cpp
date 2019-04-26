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

}

TileDefinition* TileDefinition::GetTileDefinitionByName(const std::string& name) {
    auto found_iter = s_registry.find(name);
    if(found_iter != std::end(s_registry)) {
        return found_iter->second.get();
    }
    return nullptr;
}

TileDefinition* TileDefinition::GetTileDefinitionByGlyph(char /*glyph*/) {
    //for(const auto& tile : s_registry) {
    //    if(tile.second->_glyph == glyph) {
    //        return tile.second.get();
    //    }
    //}
    return nullptr;
}

void TileDefinition::ClearTileRegistry() {
    for(auto& tile : s_registry) {
        tile.second.reset();
        tile.second;
    }
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

 bool TileDefinition::LoadFromXml(const XMLElement& /*elem*/) {
     return false;
 }
