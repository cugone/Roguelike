#include "Game/Adventure.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include "Game/Actor.hpp"
#include "Game/Map.hpp"
#include "Game/Layer.hpp"
#include "Game/Tile.hpp"

#include <sstream>

Adventure::Adventure(const XMLElement& elem) noexcept
{
    GUARANTEE_OR_DIE(LoadFromXml(elem), "Adventure failed to load.");
    _current_map_iter = std::begin(_maps);
    player = _current_map_iter->player;
}

std::vector<Map>::iterator Adventure::CurrentMap() const noexcept {
    return _current_map_iter;
}

void Adventure::NextMap() noexcept {
    if(_current_map_iter != std::end(_maps) - 1) {
        ++_current_map_iter;
        _current_map_iter->player = player;
        PlacePlayerNearEntrance();
    }
}

void Adventure::PreviousMap() noexcept {
    if(_current_map_iter != std::begin(_maps)) {
        --_current_map_iter;
        _current_map_iter->player = player;
        PlacePlayerNearExit();
    }
}

void Adventure::PlacePlayerNearEntrance() noexcept {
    const auto* placement = [this]() -> const Tile* {
        if(auto* layer = _current_map_iter->player->layer; layer != nullptr) {
            Tile* entrance_tile{nullptr};
            for(auto& tile : (*layer)) {
                if(tile.IsEntrance()) {
                    entrance_tile = &tile;
                    break;
                }
            }
            if(entrance_tile) {
                for(auto* tile : entrance_tile->GetNeighbors()) {
                    if(tile && tile->IsPassable()) {
                        return tile;
                    }
                }
            }
        }
        return nullptr;
    }(); //IIIL
    {
        const auto error_msg = [this]() {
            std::ostringstream ss{};
            ss << _current_map_iter->_name << " has no valid entrance placement.\n";
            return ss.str();
        }(); //IIIL
        GUARANTEE_OR_DIE(placement, error_msg.c_str());
    }
    _current_map_iter->player->SetPosition(placement->GetCoords());
}

void Adventure::PlacePlayerNearExit() noexcept {
    const auto* placement = [this]() -> const Tile* {
        if(auto* layer = _current_map_iter->player->layer; layer != nullptr) {
            Tile* exit_tile{nullptr};
            for(auto& tile : (*layer)) {
                if(tile.IsExit()) {
                    exit_tile = &tile;
                    break;
                }
            }
            if(exit_tile) {
                for(auto* tile : exit_tile->GetNeighbors()) {
                    if(tile->IsPassable()) {
                        return tile;
                    }
                }
            }
        }
        return nullptr;
    }(); //IIIL
    {
        const auto error_msg = [this]() {
            std::ostringstream ss{};
            ss << _current_map_iter->_name << " has no valid exit placement.\n";
            return ss.str();
        }(); //IIIL
        GUARANTEE_OR_DIE(placement, error_msg.c_str());
    }
    _current_map_iter->player->SetPosition(placement->GetCoords());
}

bool Adventure::LoadFromXml(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "adventure", "maps", "name");
    _name = DataUtils::ParseXmlAttribute(elem, "name", _name);
    if(auto* xml_maps = elem.FirstChildElement("maps"); xml_maps != nullptr) {
        const auto map_count = DataUtils::GetChildElementCount(*xml_maps, "map");
        _maps.reserve(map_count);
        DataUtils::ForEachChildElement(*xml_maps, "map", [this, map_count](const XMLElement& xml_map) {
            DataUtils::ValidateXmlElement(xml_map, "map", "", "src");
            const auto map_src = DataUtils::ParseXmlAttribute(xml_map, "src", std::string{});
            _maps.emplace_back(map_src);
            _maps.back().SetParentAdventure(this);
        });
    } else {
        DebuggerPrintf(std::format("Adventure \"{}\" contains no maps.", _name));
        return false;
    }
    return true;
}


