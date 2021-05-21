#include "Game/Adventure.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Tile.hpp"

#include <sstream>

Adventure::Adventure(Renderer& renderer, const XMLElement& elem) noexcept
    : _renderer(renderer)
{
    GUARANTEE_OR_DIE(LoadFromXml(elem), "Adventure failed to load.");
    _current_map_iter = std::begin(_maps);
    currentMap = (*_current_map_iter).get();
}

void Adventure::NextMap() noexcept {
    if(_current_map_iter != std::end(_maps) - 1) {
        ++_current_map_iter;
        currentMap = (*_current_map_iter).get();
        PlacePlayerNearEntrance();
        return;
    }
}

void Adventure::PreviousMap() noexcept {
    if(_current_map_iter != std::begin(_maps)) {
        --_current_map_iter;
        currentMap = (*_current_map_iter).get();
        currentMap->player->SetPosition(currentMap->player->tile->GetEastNeighbor()->GetCoords());
    }
}

void Adventure::PlacePlayerNearEntrance() noexcept {
    const auto* placement = [this]() -> const Tile* {
        for(auto* tile : currentMap->player->tile->GetNeighbors()) {
            if(tile->IsPassable()) {
                return tile;
            }
        }
        return nullptr;
    }(); //IIIL
    if(placement) {
        currentMap->player->SetPosition(placement->GetCoords());
    } else {
        const auto error_str = [this]() {
            std::ostringstream ss{};
            ss << currentMap->_name << " has no valid entrance placement.\n";
            return ss.str();
        }(); //IIIL
        ERROR_AND_DIE(error_str.c_str());
    }
}

bool Adventure::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "adventure", "maps", "name");
    _name = DataUtils::ParseXmlAttribute(elem, "name", _name);
    if(auto* xml_maps = elem.FirstChildElement("maps"); xml_maps != nullptr) {
        const auto map_count = DataUtils::GetChildElementCount(*xml_maps, "map");
        _maps.reserve(map_count);
        DataUtils::ForEachChildElement(*xml_maps, "map", [this, map_count](const XMLElement& xml_map) {
            DataUtils::ValidateXmlElement(xml_map, "map", "", "src");
            const auto map_src = DataUtils::ParseXmlAttribute(xml_map, "src", "");
            tinyxml2::XMLDocument doc{};
            if(auto load_result = doc.LoadFile(map_src.c_str()); load_result == tinyxml2::XML_SUCCESS) {
                auto newMap = std::make_unique<Map>(_renderer, *doc.RootElement());
                newMap->SetParentAdventure(this);
                _maps.emplace_back(std::move(newMap));
            }
        });
    } else {
        std::ostringstream ss{};
        ss << "Adventure \"" << _name << "\" contains no maps.";
        DebuggerPrintf(ss.str().c_str());
        return false;
    }
    return true;
}


