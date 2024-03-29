#include "Game/TileDefinition.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"

#include "Engine/Renderer/AnimatedSprite.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

#include "Engine/Services/ServiceLocator.hpp"
#include "Engine/Services/IRendererService.hpp"

#include "Game/GameCommon.hpp"

#include <memory>


TileDefinition* TileDefinition::CreateOrGetTileDefinition(const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet) {
    auto new_def = std::make_unique<TileDefinition>(elem, sheet);
    auto* new_def_ptr = new_def.get();
    std::string new_def_name = new_def->name;
    if(auto found = s_registry.find(new_def_name); found != std::end(s_registry)) {
        new_def_ptr = found->second.get();
        return new_def_ptr;
    } else {
        s_registry.try_emplace(new_def_name, std::move(new_def));
    }
    return new_def_ptr;
}

TileDefinition* TileDefinition::CreateTileDefinition(const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet) {
    auto new_def = std::make_unique<TileDefinition>(elem, sheet);
    auto* new_def_ptr = new_def.get();
    std::string new_def_name = new_def->name;
    s_registry.try_emplace(new_def_name, std::move(new_def));
    return new_def_ptr;
}

void TileDefinition::ClearTileDefinitions() {
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

TileDefinition* TileDefinition::GetTileDefinitionByIndex(std::size_t index) {
    for(const auto& tile : s_registry) {
        if(tile.second->GetIndex() == index) {
            return tile.second.get();
        }
    }
    return nullptr;
}

uint32_t TileDefinition::GetLightingBits() const noexcept {
    if(is_opaque && is_solid) {
        return tile_flags_opaque_mask | tile_flags_solid_mask;
    } else if(is_opaque) {
        return tile_flags_opaque_mask;
    } else if(is_solid) {
        return tile_flags_solid_mask;
    } else {
        return uint32_t{0u};
    }
}

const Texture* TileDefinition::GetTexture() const {
    return GetSheet()->GetTexture();
}

Texture* TileDefinition::GetTexture() {
    return const_cast<Texture*>(static_cast<const TileDefinition&>(*this).GetTexture());
}

const SpriteSheet* TileDefinition::GetSheet() const {
    return _sheet.get();
}

SpriteSheet* TileDefinition::GetSheet() {
    return const_cast<SpriteSheet*>(static_cast<const TileDefinition&>(*this).GetSheet());
}

const AnimatedSprite* TileDefinition::GetSprite() const {
    return _sprite.get();
}

AnimatedSprite* TileDefinition::GetSprite() {
    return _sprite.get();
}

IntVector2 TileDefinition::GetIndexCoords() const {
    return _index;
}

std::size_t TileDefinition::GetIndex() const {
    if(auto* sheet = GetSheet()) {
        return (_index.x + _random_index_offset) + _index.y * sheet->GetLayout().x;
    }
    return std::size_t(-1);
}

TileDefinition::TileDefinition(const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet)
: _sheet(sheet)
{
    GUARANTEE_OR_DIE(LoadFromXml(elem), "TileDefinition failed to load.\n");
}

bool TileDefinition::LoadFromXml(const XMLElement& elem) {

    DataUtils::ValidateXmlElement(elem, "tileDefinition", "glyph", "name,index", "opaque,solid,visible,invisible,allowDiagonalMovement,animation,offset,entrance,exit,light,selflight");

    name = DataUtils::ParseXmlAttribute(elem, "name", name);
    if(auto xml_randomOffset = elem.FirstChildElement("offset"); xml_randomOffset != nullptr) {
        _random_index_offset = DataUtils::ParseXmlAttribute(*xml_randomOffset, "value", _random_index_offset);
    }
    _index = DataUtils::ParseXmlAttribute(elem, "index", _index);
    AddOffsetToIndex(_random_index_offset);

    auto xml_glyph = elem.FirstChildElement("glyph");
    glyph = DataUtils::ParseXmlAttribute(*xml_glyph, "value", glyph);

    if(auto xml_opaque = elem.FirstChildElement("opaque"); xml_opaque != nullptr) {
        is_opaque = true;
        is_opaque = DataUtils::ParseXmlAttribute(*xml_opaque, "value", is_opaque);
    }

    if(auto xml_solid = elem.FirstChildElement("solid"); xml_solid != nullptr) {
        is_solid = true;
        is_solid = DataUtils::ParseXmlAttribute(*xml_solid, "value", is_solid);
    }

    if(auto xml_visible = elem.FirstChildElement("visible"); xml_visible != nullptr) {
        is_visible = true;
        is_visible = DataUtils::ParseXmlAttribute(*xml_visible, "value", is_visible);
    }
    if(auto xml_invisible = elem.FirstChildElement("invisible"); xml_invisible != nullptr) {
        is_visible = false;
        is_visible = DataUtils::ParseXmlAttribute(*xml_invisible, "value", is_visible);
    }

    if(auto xml_diag = elem.FirstChildElement("allowDiagonalMovement"); xml_diag != nullptr) {
        allow_diagonal_movement = true;
        allow_diagonal_movement = DataUtils::ParseXmlAttribute(*xml_diag, "value", allow_diagonal_movement);
    }
    
    if(auto xml_enter = elem.FirstChildElement("entrance"); xml_enter != nullptr) {
        is_entrance = true;
        is_entrance = DataUtils::ParseXmlAttribute(*xml_enter, "value", is_entrance);
    }

    if(auto xml_exit = elem.FirstChildElement("exit"); xml_exit != nullptr) {
        is_exit = true;
        is_exit = DataUtils::ParseXmlAttribute(*xml_exit, "value", is_exit);
    }

    if(auto xml_light = elem.FirstChildElement("light"); xml_light != nullptr) {
        light = DataUtils::ParseXmlAttribute(*xml_light, "value", light);
    }
    if(auto xml_selflight = elem.FirstChildElement("selflight"); xml_selflight != nullptr) {
        self_illumination = DataUtils::ParseXmlAttribute(*xml_selflight, "value", self_illumination);
    }
    auto* renderer = ServiceLocator::get<IRendererService>();
    if(auto xml_animation = elem.FirstChildElement("animation"); xml_animation != nullptr) {
        is_animated = true;
        _sprite = std::move(renderer->CreateAnimatedSprite(_sheet, *xml_animation));
    } else {
        _sprite = std::move(renderer->CreateAnimatedSprite(_sheet, _index));
    }
    return true;
}

void TileDefinition::SetIndex(const IntVector2& indexCoords) {
    _index = indexCoords;
}

void TileDefinition::SetIndex(std::size_t index) {
    if(auto sheet = GetSheet()) {
        const auto& layout = sheet->GetLayout();
        const auto x = static_cast<int>(index % static_cast<std::size_t>(layout.x));
        const auto y = static_cast<int>(index / static_cast<std::size_t>(layout.x));
        SetIndex(x, y);
    }
}

void TileDefinition::SetIndex(int x, int y) {
    SetIndex(IntVector2{x, y});
}

void TileDefinition::AddOffsetToIndex(std::size_t offset) {
    const auto x = static_cast<int>(_index.x + offset);
    const auto y = static_cast<int>(_index.y);
    SetIndex(x, y);
}
