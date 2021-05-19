#include "Game/MapGenerator.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Game/Actor.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/Item.hpp"
#include "Game/Feature.hpp"
#include "Game/TileDefinition.hpp"
#include "Game/Pathfinder.hpp"

#include "Game/PursueBehavior.hpp"

#include <cmath>

MapGenerator::MapGenerator(Map* map, const XMLElement& elem) noexcept
    : _xml_element(elem)
    , _map(map)
{ /* DO NOTHING */
}

void MapGenerator::LoadLayers(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "layers", "layer", "");
    std::size_t layer_count = DataUtils::GetChildElementCount(elem, "layer");
    if(layer_count > _map->max_layers) {
        const auto ss = std::string{"Layer count of map "} + _map->_name + " is greater than the maximum allowed (" + std::to_string(_map->max_layers) + ")."
            "\nOnly the first " + std::to_string(_map->max_layers) + " layers will be used.";
        g_theFileLogger->LogLine(ss);
    }

    auto layer_index = 0;
    _map->_layers.reserve(layer_count);
    DataUtils::ForEachChildElement(elem, "layer",
        [this, &layer_index](const XMLElement& xml_layer) {
            if(static_cast<std::size_t>(layer_index) < _map->max_layers) {
                _map->_layers.emplace_back(std::make_unique<Layer>(_map, xml_layer));
                _map->_layers.back()->z_index = layer_index++;
            }
        });
    _map->_layers.shrink_to_fit();
}

HeightMapGenerator::HeightMapGenerator(Map* map, const XMLElement& elem) noexcept
    : MapGenerator(map, elem)
{
    /* DO NOTHING */
}

void HeightMapGenerator::Generate() {
    DataUtils::ValidateXmlElement(_xml_element, "mapGenerator", "glyph", "type,src");
    const auto xml_src = DataUtils::ParseXmlAttribute(_xml_element, "src", "");
    Image img(std::filesystem::path{xml_src});
    const auto width = img.GetDimensions().x;
    const auto height = img.GetDimensions().y;
    _map->_layers.emplace_back(std::make_unique<Layer>(_map, img));
    auto* layer = _map->_layers.back().get();
    for(auto& t : *layer) {
        int closest_height = 257;
        char smallest_value = ' ';
        DataUtils::ForEachChildElement(_xml_element, "glyph",
            [&t, &closest_height, &smallest_value, layer](const XMLElement& elem) {
                const auto glyph_value = DataUtils::ParseXmlAttribute(elem, "value", ' ');
                const auto glyph_height = DataUtils::ParseXmlAttribute(elem, "height", 0);
                if(t.color.r <= glyph_height) {
                    closest_height = glyph_height;
                    smallest_value = glyph_value;
                }
            });
        t.ChangeTypeFromGlyph(smallest_value);
        t.color = Rgba::White;
        t.layer = layer;
    }
    //TODO: Implement multiple layers for height maps
    layer->z_index = 0;
}

void HeightMapGenerator::LoadItems(const XMLElement& elem) {
    _map->LoadItemsForMap(elem);
}

void HeightMapGenerator::LoadActors(const XMLElement& elem) {
    _map->LoadActorsForMap(elem);
}

void HeightMapGenerator::LoadFeatures(const XMLElement& elem) {
    _map->LoadFeaturesForMap(elem);
}

FileMapGenerator::FileMapGenerator(Map* map, const XMLElement& elem) noexcept
    : MapGenerator(map, elem)
{
    /* DO NOTHING */
}

void FileMapGenerator::Generate() {
    DataUtils::ValidateXmlElement(_xml_element, "mapGenerator", "", "src", "", "");
    LoadLayersFromFile(_xml_element);
}

void FileMapGenerator::LoadLayersFromFile(const XMLElement& elem) {
    const auto xml_src = DataUtils::ParseXmlAttribute(elem, "src", "");
    if(auto src = FileUtils::ReadStringBufferFromFile(xml_src)) {
        if(src.value().empty()) {
            ERROR_AND_DIE("Loading Map from file with empty or invalid source attribute.");
        }
        tinyxml2::XMLDocument doc;
        if(tinyxml2::XML_SUCCESS == doc.Parse(src.value().c_str(), src.value().size())) {
            auto* xml_layers = doc.RootElement();
            LoadLayers(*xml_layers);
        }
    }
}

void FileMapGenerator::LoadItems(const XMLElement& elem) {
    _map->LoadItemsForMap(elem);
}

void FileMapGenerator::LoadActors(const XMLElement& elem) {
    _map->LoadActorsForMap(elem);
}

void FileMapGenerator::LoadFeatures(const XMLElement& elem) {
    _map->LoadFeaturesForMap(elem);
}

XmlMapGenerator::XmlMapGenerator(Map* map, const XMLElement& elem) noexcept
    : MapGenerator(map, elem)
{
    /* DO NOTHING */
}

void XmlMapGenerator::Generate() {
    DataUtils::ValidateXmlElement(_xml_element, "mapGenerator", "layers", "");
    LoadLayersFromXml(_xml_element);
    const auto map_dims = _map->CalcMaxDimensions();
    const auto map_width = static_cast<int>(map_dims.x);
    const auto map_height = static_cast<int>(map_dims.y);
    _map->GetPathfinder()->Initialize(map_width, map_height);
    LoadFeatures(_map->_root_xml_element);
    LoadActors(_map->_root_xml_element);
    LoadItems(_map->_root_xml_element);
}

void XmlMapGenerator::LoadLayersFromXml(const XMLElement& elem) {
    if(auto xml_layers = elem.FirstChildElement("layers")) {
        LoadLayers(*xml_layers);
    }
}


void XmlMapGenerator::LoadItems(const XMLElement& elem) {
    _map->LoadItemsForMap(elem);
}

void XmlMapGenerator::LoadActors(const XMLElement& elem) {
    _map->LoadActorsForMap(elem);
}

void XmlMapGenerator::LoadFeatures(const XMLElement& elem) {
    _map->LoadFeaturesForMap(elem);
}

MazeMapGenerator::MazeMapGenerator(Map* map, const XMLElement& elem) noexcept
    : MapGenerator(map, elem)
{
    /* DO NOTHING */
}

void MazeMapGenerator::Generate(Map* map, const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "mapGenerator", "", "algorithm");
    const auto algoName = DataUtils::ParseXmlAttribute(elem, "algorithm", "");
    GUARANTEE_OR_DIE(!algoName.empty(), "Maze Generator algorithm type specifier cannot be empty.");
    if(algoName == "rooms") {
        map->_map_generator = std::make_unique<RoomsMapGenerator>(map, elem);
        map->_map_generator->Generate();
    } else if(algoName == "roomsAndCorridors") {
        map->_map_generator = std::make_unique<RoomsAndCorridorsMapGenerator>(map, elem);
        map->_map_generator->Generate();
    }
}

RoomsMapGenerator::RoomsMapGenerator(Map* map, const XMLElement& elem) noexcept
    : MazeMapGenerator(map, elem)
{
    /* DO NOTHING */
}

void RoomsMapGenerator::Generate() {
    DataUtils::ValidateXmlElement(_xml_element, "mapGenerator", "minSize,maxSize", "count,floor,wall,default", "", "width,height");
    const auto min_size = std::clamp([&]()->const int { const auto* xml_min = _xml_element.FirstChildElement("minSize"); int result = DataUtils::ParseXmlElementText(*xml_min, 1); if(result < 0) result = 1; return result; }(), 1, Map::max_dimension); //IIIL
    const auto max_size = std::clamp([&]()->const int { const auto* xml_max = _xml_element.FirstChildElement("maxSize"); int result = DataUtils::ParseXmlElementText(*xml_max, 1); if(result < 0) result = 1; return result; }(), 1, Map::max_dimension); //IIIL
    const int room_count = DataUtils::ParseXmlAttribute(_xml_element, "count", 1);
    const unsigned int width = DataUtils::ParseXmlAttribute(_xml_element, "width", 0u);
    const unsigned int height = DataUtils::ParseXmlAttribute(_xml_element, "height", 0u);
    rooms.reserve(room_count);
    for(int i = 0; i < room_count; ++i) {
        const auto w = MathUtils::GetRandomIntInRange(min_size, max_size);
        const auto h = MathUtils::GetRandomIntInRange(min_size, max_size);
        const auto x = MathUtils::GetRandomIntInRange(w, Map::max_dimension - 2 * w);
        const auto y = MathUtils::GetRandomIntInRange(h, Map::max_dimension - 2 * h);
        rooms.push_back(AABB2{Vector2{(float)x, (float)y}, (float)w, (float)h});
    }
    for(int i = 0; i < room_count - 1; ++i) {
        for(int j = 1; j < room_count; ++j) {
            if(i == j) continue;
            auto& room1 = rooms[i];
            auto& room2 = rooms[j];
            if(MathUtils::DoAABBsOverlap(room1, room2)) {
                const auto dispToR1 = room1.CalcCenter() - room2.CalcCenter();
                const auto dispToR2 = -dispToR1;
                const auto dirToR1 = dispToR1.GetNormalize();
                const auto dirToR2 = -dirToR1;
                rooms[i].Translate(dirToR1 * dispToR1);
                rooms[j].Translate(dirToR2 * dispToR2);
            }
        }
    }
    AABB2 world_bounds;
    world_bounds.maxs.x = static_cast<float>(width);
    world_bounds.maxs.y = static_cast<float>(height);
    for(const auto& room : rooms) {
        world_bounds.StretchToIncludePoint(room.mins - Vector2::ONE);
        world_bounds.StretchToIncludePoint(room.maxs + Vector2::ONE);
    }
    const auto map_width = static_cast<int>(world_bounds.CalcDimensions().x);
    const auto map_height = static_cast<int>(world_bounds.CalcDimensions().y);
    if(_map->_layers.empty()) {
        _map->_layers.emplace_back(std::make_unique<Layer>(_map, IntVector2{map_width, map_height}));
    } else {
        if(_map->_layers[0]->tileDimensions != IntVector2{map_width, map_height}) {
            _map->_layers[0] = std::move(std::make_unique<Layer>(_map, IntVector2{map_width, map_height}));
        }
    }
    floorType = DataUtils::ParseXmlAttribute(_xml_element, "floor", floorType);
    wallType = DataUtils::ParseXmlAttribute(_xml_element, "wall", wallType);
    defaultType = DataUtils::ParseXmlAttribute(_xml_element, "default", defaultType);
    stairsDownType = DataUtils::ParseXmlAttribute(_xml_element, "down", stairsDownType);
    stairsUpType = DataUtils::ParseXmlAttribute(_xml_element, "up", stairsUpType);
    enterType = DataUtils::ParseXmlAttribute(_xml_element, "enter", enterType);
    exitType = DataUtils::ParseXmlAttribute(_xml_element, "exit", exitType);

    if(auto* layer = _map->GetLayer(0); layer != nullptr) {
        for(auto& tile : *layer) {
            tile.ChangeTypeFromName(defaultType);
        }
    }
    for(auto& room : rooms) {
        const auto roomWallTiles = _map->GetTilesInArea(room);
        for(auto& tile : roomWallTiles) {
            if(tile) {
                tile->ChangeTypeFromName(wallType);
            }
        }
    }
    for(auto& room : rooms) {
        const auto room_floor_bounds = [&]() { AABB2 bounds = room; bounds.AddPaddingToSides(-1.0f, -1.0f); return bounds; }();
        const auto roomFloorTiles = _map->GetTilesInArea(room_floor_bounds);
        for(auto& tile : roomFloorTiles) {
            if(tile) {
                tile->ChangeTypeFromName(floorType);
            }
        }
    }
}

void RoomsMapGenerator::LoadItems(const XMLElement& elem) {
    if(auto* xml_items = elem.FirstChildElement("items")) {
        DataUtils::ValidateXmlElement(*xml_items, "items", "item", "");
        DataUtils::ForEachChildElement(*xml_items, "item", [this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "item", "", "name,position");
            const auto name = DataUtils::ParseXmlAttribute(elem, "name", nullptr);
            const auto pos = DataUtils::ParseXmlAttribute(elem, "position", IntVector2{-1, -1});
            if(auto* tile = _map->GetTile(IntVector3(pos, 0))) {
                tile->AddItem(Item::GetItem(name));
            } else {
                //TODO Add StringUtils::to_string(const IntVector2/3/4&);
                std::ostringstream ss;
                ss << "Invalid tile " << pos << " for item \"" << name << "\" placement.";
                g_theFileLogger->LogLineAndFlush(ss.str());
            }
            });
    }
}

void RoomsMapGenerator::LoadActors(const XMLElement& elem) {
    if(auto* xml_actors = elem.FirstChildElement("actors")) {
        DataUtils::ValidateXmlElement(*xml_actors, "actors", "actor", "");
        DataUtils::ForEachChildElement(*xml_actors, "actor",
            [this](const XMLElement& elem) {
                auto* actor = Actor::CreateActor(_map, elem);
                auto actor_name = StringUtils::ToLowerCase(actor->name);
                bool is_player = actor_name == "player";
                if(_map->player && is_player) {
                    ERROR_AND_DIE("Map failed to load. Multiplayer not yet supported.");
                }
                actor->SetFaction(Faction::Enemy);
                if(is_player) {
                    _map->player = actor;
                    _map->player->SetFaction(Faction::Player);
                }
                _map->_entities.push_back(actor);
                _map->_actors.push_back(actor);
            });
    }
}

void RoomsMapGenerator::LoadFeatures(const XMLElement& elem) {
    if(auto* xml_features = elem.FirstChildElement("features")) {
        DataUtils::ValidateXmlElement(*xml_features, "features", "feature", "");
        DataUtils::ForEachChildElement(*xml_features, "feature",
            [this](const XMLElement& elem) {
                auto* feature = Feature::CreateFeature(_map, elem);
                _map->_entities.push_back(feature);
                _map->_features.push_back(feature);
            });
    }
}

RoomsAndCorridorsMapGenerator::RoomsAndCorridorsMapGenerator(Map* map, const XMLElement& elem) noexcept
    : RoomsMapGenerator(map, elem)
{
    /* DO NOTHING */
}

void RoomsAndCorridorsMapGenerator::LoadItems(const XMLElement& elem) {
    if(auto* xml_items = elem.FirstChildElement("items")) {
        DataUtils::ValidateXmlElement(*xml_items, "items", "item", "");
        DataUtils::ForEachChildElement(*xml_items, "item", [this](const XMLElement& elem) {
            DataUtils::ValidateXmlElement(elem, "item", "", "name", "", "position");
            const auto name = DataUtils::ParseXmlAttribute(elem, "name", nullptr);
            const auto pos = DataUtils::ParseXmlAttribute(elem, "position", IntVector2{-1, -1});
            if(auto* tile = _map->GetTile(IntVector3(pos, 0))) {
                tile->AddItem(Item::GetItem(name));
            }
            });
    }
}

void RoomsAndCorridorsMapGenerator::LoadActors(const XMLElement& elem) {
    if(auto* xml_actors = elem.FirstChildElement("actors")) {
        DataUtils::ValidateXmlElement(*xml_actors, "actors", "actor", "");
        DataUtils::ForEachChildElement(*xml_actors, "actor",
            [this](const XMLElement& elem) {
                auto* actor = Actor::CreateActor(_map, elem);
                auto actor_name = StringUtils::ToLowerCase(actor->name);
                bool is_player = actor_name == "player";
                if(_map->player && is_player) {
                    ERROR_AND_DIE("Map failed to load. Multiplayer not yet supported.");
                }
                actor->SetFaction(Faction::Enemy);
                if(is_player) {
                    _map->player = actor;
                    _map->player->SetFaction(Faction::Player);
                }
                _map->_entities.push_back(actor);
                _map->_actors.push_back(actor);
            });
    }
}

void RoomsAndCorridorsMapGenerator::LoadFeatures(const XMLElement& elem) {
    if(auto* xml_features = elem.FirstChildElement("features")) {
        DataUtils::ValidateXmlElement(*xml_features, "features", "feature", "");
        DataUtils::ForEachChildElement(*xml_features, "feature",
            [this](const XMLElement& elem) {
                auto* feature = Feature::CreateFeature(_map, elem);
                _map->_entities.push_back(feature);
                _map->_features.push_back(feature);
            });
    }
}

void RoomsAndCorridorsMapGenerator::Generate() {
    do {
        RoomsMapGenerator::Generate();
        GenerateCorridors();
        const auto map_dims = _map->CalcMaxDimensions();
        const auto map_width = static_cast<int>(map_dims.x);
        const auto map_height = static_cast<int>(map_dims.y);
        _map->GetPathfinder()->Initialize(map_width, map_height);
    } while(!GenerateExitAndEntrance());
    LoadFeatures(_map->_root_xml_element);
    LoadActors(_map->_root_xml_element);
    LoadItems(_map->_root_xml_element);
    PlaceActors();
    PlaceFeatures();
    PlaceItems();
}

void RoomsAndCorridorsMapGenerator::GenerateCorridors() noexcept {
    const auto roomCount = rooms.size();
    for(auto i = std::size_t{0u}; i < roomCount; ++i) {
        const auto& r1 = rooms[i % roomCount];
        const auto& r2 = rooms[(i + 1u) % roomCount];
        const auto r1pos = r1.CalcCenter();
        const auto r1dims = r1.CalcDimensions();
        const auto r2pos = r2.CalcCenter();
        const auto r2dims = r2.CalcDimensions();
        const auto horizontal_first = MathUtils::GetRandomBool();
        if(horizontal_first) {
            MakeHorizontalCorridor(r1, r2);
            MakeVerticalCorridor(r2, r1);
        } else {
            MakeVerticalCorridor(r1, r2);
            MakeHorizontalCorridor(r2, r1);
        }
    }
    //Fill areas of conjoined rooms
    for(auto& room : rooms) {
        const auto room_floor_bounds = [&]() { AABB2 bounds = room; bounds.AddPaddingToSides(-1.0f, -1.0f); return bounds; }();
        const auto roomFloorTiles = _map->GetTilesInArea(room_floor_bounds);
        for(auto& tile : roomFloorTiles) {
            if(tile) {
                tile->ChangeTypeFromName(floorType);
            }
        }
    }
}

void RoomsAndCorridorsMapGenerator::MakeVerticalCorridor(const AABB2& from, const AABB2& to) noexcept {
    auto start = from.CalcCenter().y;
    auto end = to.CalcCenter().y;
    if(end < start) {
        std::swap(end, start);
    }
    const auto step = std::signbit(end - start) ? -1.0f : 1.0f;
    const auto x = from.CalcCenter().x;
    const auto can_be_corridor_wall = [&](const std::string& name) { return name != this->floorType; };
    for(auto y = start; y <= end; y += step) {
        if(auto* tile = _map->GetTile(IntVector3{static_cast<int>(x), static_cast<int>(y), 0})) {
            tile->ChangeTypeFromName(floorType);
            const auto neighbors = tile->GetNeighbors();
            for(auto* neighbor : neighbors) {
                if(neighbor && can_be_corridor_wall(neighbor->GetDefinition()->name)) {
                    neighbor->ChangeTypeFromName(wallType);
                }
            }
        }
    }
}

void RoomsAndCorridorsMapGenerator::MakeHorizontalCorridor(const AABB2& from, const AABB2& to) noexcept {
    auto start = from.CalcCenter().x;
    auto end = to.CalcCenter().x;
    if(end < start) {
        std::swap(end, start);
    }
    const auto step = std::signbit(end - start) ? -1.0f : 1.0f;
    const auto y = from.CalcCenter().y;
    const auto can_be_corridor_wall = [&](const std::string& name) { return name != this->floorType; };
    for(auto x = start; x <= end; x += step) {
        if(auto* tile = _map->GetTile(IntVector3{static_cast<int>(x), static_cast<int>(y), 0})) {
            tile->ChangeTypeFromName(floorType);
            const auto neighbors = tile->GetNeighbors();
            for(auto* neighbor : neighbors) {
                if(neighbor && can_be_corridor_wall(neighbor->GetDefinition()->name)) {
                    neighbor->ChangeTypeFromName(wallType);
                }
            }
        }
    }
}

bool RoomsAndCorridorsMapGenerator::VerifyExitIsReachable(const IntVector2& enter_loc, const IntVector2& exit_loc) const noexcept {
    const auto viable = [this](const IntVector2& a)->bool {
        return this->_map->IsTilePassable(a);
    };
    const auto h = [](const IntVector2& a, const IntVector2& b) {
        return MathUtils::CalculateManhattanDistance(a, b);
    };
    const auto d = [this](const IntVector2& a, const IntVector2& b) {
        if(a.x == b.x || a.y == b.y) return 10; //Distance of 1 times ten.
        return 14; //Euclidian diagonal distance times 10 casted to an integer.
    };
    auto* pather = this->_map->GetPathfinder();
    if(const auto result = pather->AStar(enter_loc, exit_loc, viable, h, d); result == Pathfinder::PATHFINDING_SUCCESS) {
        return true;
    } else {
        return false;
    }
}

void RoomsAndCorridorsMapGenerator::PlaceActors() noexcept {
    const int room_count = static_cast<int>(rooms.size());
    auto closed_set = std::vector<std::size_t>{};
    closed_set.reserve(room_count);
    for(auto* actor : _map->_actors) {
        const auto room_idx = [&]() {
            auto idx = static_cast<std::size_t>(MathUtils::GetRandomIntLessThan(room_count));
            while(std::find(std::begin(closed_set), std::end(closed_set), idx) != std::end(closed_set)) {
                idx = static_cast<std::size_t>(MathUtils::GetRandomIntLessThan(room_count));
            }
            closed_set.push_back(idx);
            return idx;
        }();
        actor->SetPosition(IntVector2{rooms[room_idx].CalcCenter()});
        if(auto* b = actor->GetCurrentBehavior(); b && b->GetName() == "pursue") {
            if(auto* bAsPursue = dynamic_cast<PursueBehavior*>(b)) {
                bAsPursue->SetTarget(_map->player);
            }
        }
    }
}

void RoomsAndCorridorsMapGenerator::PlaceFeatures() noexcept {
    const auto map_dims = _map->CalcMaxDimensions();
    for(auto* feature : _map->_features) {
        const auto x = MathUtils::GetRandomIntLessThan(static_cast<int>(map_dims.x));
        const auto y = MathUtils::GetRandomIntLessThan(static_cast<int>(map_dims.y));
        const auto tile_pos = IntVector2{x, y};
        feature->SetPosition(tile_pos);
    }
}

void RoomsAndCorridorsMapGenerator::PlaceItems() noexcept {
    /* DO NOTHING */
}

bool RoomsAndCorridorsMapGenerator::GenerateExitAndEntrance() noexcept {
    IntVector2 start{};
    IntVector2 end{};
    std::set<std::pair<int, int>> closed_set{};
    do {
        {
            const std::size_t roomCount = rooms.size();
            if(closed_set.size() >= (roomCount * roomCount) - roomCount) {
                rooms.clear();
                return false;
            }
        }
        auto up_id = 0;
        auto down_id = 0;
        {
            bool needs_restart = false;
            do {
                const auto roomCountAsInt = static_cast<int>(rooms.size());
                const auto room_id_with_down = MathUtils::GetRandomIntLessThan(roomCountAsInt);
                const auto room_id_with_up = [roomCountAsInt, room_id_with_down]()->int {
                    auto result = MathUtils::GetRandomIntLessThan(roomCountAsInt);
                    while(result == room_id_with_down) {
                        result = MathUtils::GetRandomIntLessThan(roomCountAsInt);
                    }
                    return result;
                }();
                const auto is_in_dtou = closed_set.find(std::make_pair(room_id_with_down, room_id_with_up)) != std::end(closed_set);
                const auto is_in_utod = closed_set.find(std::make_pair(room_id_with_up, room_id_with_down)) != std::end(closed_set);
                const auto is_in_set = is_in_dtou || is_in_utod;
                needs_restart = is_in_set;
                up_id = room_id_with_up;
                down_id = room_id_with_down;
            } while(needs_restart);
        }
        closed_set.insert(std::make_pair(down_id, up_id));
        closed_set.insert(std::make_pair(up_id, down_id));
        const auto& room_with_exit = rooms[down_id];
        const auto& room_with_entrance = rooms[up_id];
        const auto exit_loc = IntVector2{room_with_exit.mins + Vector2::ONE};
        const auto enter_loc = IntVector2{room_with_entrance.mins + Vector2::ONE};
        start = enter_loc;
        end = exit_loc;
    } while(!VerifyExitIsReachable(start, end));

    if(auto* tile = _map->GetTile(IntVector3{start, 0})) {
        tile->ChangeTypeFromName(stairsDownType);
    }
    if(auto* tile = _map->GetTile(IntVector3{end, 0})) {
        tile->ChangeTypeFromName(stairsUpType);
    }
    return true;
}
