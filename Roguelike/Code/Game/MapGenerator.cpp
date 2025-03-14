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

#include "Game/TmxReader.hpp"

#include <cmath>
#include <random>
#include <utility>

MapGenerator::MapGenerator(Map* map, XMLElement* elem) noexcept
    : _xml_element(elem)
    , _map(map)
{ /* DO NOTHING */
}

void MapGenerator::LoadLayers(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "layers", "layer", "");
    std::size_t layer_count = DataUtils::GetChildElementCount(elem, "layer");
    if(layer_count > _map->max_layers) {
        const auto err_str = std::format("Layer count of map {0} is greater than the maximum allowed ({1}).\nOnly the first {1} layers will be used.", _map->_name, _map->max_layers);
        g_theFileLogger->LogLine(err_str);
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
}

void MapGenerator::LoadItems(const XMLElement& elem) noexcept {
    _map->LoadItemsForMap(elem);
}

void MapGenerator::LoadActors(const XMLElement& elem) noexcept {
    _map->LoadActorsForMap(elem);
}

void MapGenerator::LoadFeatures(const XMLElement& elem) noexcept {
    _map->LoadFeaturesForMap(elem);
}

void MapGenerator::PlaceActors() noexcept {
    auto open_set = [this]() {
        auto result = std::vector<std::size_t>{};
        result.resize(rooms.size());
        std::iota(std::begin(result), std::end(result), std::size_t{ 0u });
        std::random_device rd;
        std::mt19937_64 g(rd());
        std::shuffle(std::begin(result), std::end(result), g);
        return result;
    }(); //IIIL
    for(auto* actor : _map->_actors) {
        if(open_set.empty()) {
            break;
        }
        const auto room_idx = [&]() {
            auto idx = open_set.back();
            open_set.pop_back();
            return idx;
        }();
        actor->SetPosition(IntVector2{ rooms[room_idx].CalcCenter() });
        if(auto* b = actor->GetCurrentBehavior(); b && b->GetName() == "pursue") {
            if(auto* bAsPursue = dynamic_cast<PursueBehavior*>(b)) {
                bAsPursue->SetTarget(_map->player);
            }
        }
    }
    //TODO: Determine enter/exit tile
    //player->SetPosition(_map->GetTile());
}

void MapGenerator::PlaceFeatures() noexcept {
    const auto map_dims = _map->CalcMaxDimensions();
    for(auto* feature : _map->_features) {
        const auto x = MathUtils::GetRandomLessThan(map_dims.x);
        const auto y = MathUtils::GetRandomLessThan(map_dims.y);
        const auto tile_pos = IntVector2{ Vector2{x, y} };
        feature->SetPosition(tile_pos);
    }
}

void MapGenerator::PlaceItems() noexcept {
    /* DO NOTHING */
}

void MapGenerator::SetRootXmlElement(const XMLElement& root_element) noexcept {
    _xml_element = const_cast<XMLElement*>(&root_element);
}

void MapGenerator::SetParentMap(Map* map) noexcept {
    if(map) {
        _map = map;
    }
}

void MapGenerator::Generate() noexcept {
    DataUtils::ValidateXmlElement(*_xml_element, "mapGenerator", "", "type");
    DataUtils::ValidateXmlAttribute(*_xml_element, "type", "heightmap,file,maze,xml");
    const auto type = DataUtils::GetAttributeAsString(*_xml_element, "type");
    const auto isHeightMap = type == std::string_view{ "heightmap" };
    const auto isFileMap = type == std::string_view{ "file" };
    const auto isMazeMap = type == std::string_view{ "maze" };
    const auto isXmlMap = type == std::string_view{ "xml" };
    if(isHeightMap) {
        GenerateFromHeightMap();
    } else if(isFileMap) {
        GenerateFromFile();
    } else if(isMazeMap) {
        GenerateMaze();
    } else if(isXmlMap) {
        GenerateFromEmbeddedXml();
    } else {
        const auto msg = std::format("Could not generate map: \"{}\" is not a known type.", type);
        ERROR_AND_DIE(msg.c_str());
    }
    LoadFeatures(*_map->_root_xml_element);
    LoadActors(*_map->_root_xml_element);
    LoadItems(*_map->_root_xml_element);

}

void MapGenerator::GenerateFromEmbeddedXml() noexcept {
    DataUtils::ValidateXmlElement(*_xml_element, "mapGenerator", "layers", "");
    if(auto xml_layers = _xml_element->FirstChildElement("layers")) {
        LoadLayers(*xml_layers);
    }
    _map->GetPathfinder()->Initialize(IntVector2{ _map->CalcMaxDimensions() });
}

void MapGenerator::GenerateFromHeightMap() noexcept {
    const auto src = DataUtils::ParseXmlAttribute(*_xml_element, "src", std::string{});
    Image img(std::filesystem::path{ src });
    _map->_layers.emplace_back(std::make_unique<Layer>(_map, img));
    auto* layer = _map->_layers.back().get();
    for(auto& t : *layer) {
        int closest_height = 257;
        char smallest_value = ' ';
        DataUtils::ForEachChildElement(*_xml_element, "glyph",
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
    layer->z_index = 0;
}

void MapGenerator::GenerateFromFile() noexcept {
    DataUtils::ValidateXmlElement(*_xml_element, "mapGenerator", "", "src", "", "");
    const auto src = DataUtils::ParseXmlAttribute(*_xml_element, "src", std::string{});
    GUARANTEE_OR_DIE(!src.empty(), "Loading Map from file with empty or invalid source attribute.");
    if(auto path = std::filesystem::path{ src }; std::filesystem::exists(path) && path.has_extension()) {
        std::error_code ec{};
        if(path = std::filesystem::canonical(path, ec); ec) {
            const auto msg = std::format("Error canonicalizing path: \"{}\"", path.string());
            ERROR_AND_DIE(msg.c_str());
        }
        const auto isTmx = path.extension() == ".tmx";
        const auto isMap = path.extension() == ".map";
        const auto isXml = path.extension() == ".xml";
        if(isTmx) {
            GenerateFromTmxFile(path);
        } else if(isMap) {
            GenerateFromBinFile(path);
        } else if(isXml) {
            GenerateFromXmlFile(path);
        } else {
            const auto msg = std::format("Error: {} is not a valid map file extension.", path.extension().string());
            ERROR_AND_DIE(msg.c_str());
        }
    }
}

void MapGenerator::GenerateFromXmlFile(const std::filesystem::path& path) noexcept {
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS == doc.LoadFile(path.string().c_str())) {
        auto* xml_layers = doc.RootElement();
        LoadLayers(*xml_layers);
    }
}

void MapGenerator::GenerateFromTmxFile(const std::filesystem::path& path) noexcept {
    TmxReader reader{ path };
    reader.Parse(*_map);
}

void MapGenerator::GenerateFromBinFile(const std::filesystem::path& /*path*/) noexcept {
    //Run-length encoding for tiles.
    
}

void MapGenerator::GenerateMaze() noexcept {
    DataUtils::ValidateXmlElement(*_xml_element, "mapGenerator", "", "algorithm");
    const auto algoName = DataUtils::ParseXmlAttribute(*_xml_element, "algorithm", std::string{});
    GUARANTEE_OR_DIE(!algoName.empty(), "Maze Generator algorithm type specifier cannot be empty.");
    if(algoName == "rooms") {
        GenerateRandomRooms();
        return;
    } else if(algoName == "roomsOnly") {
        GenerateRooms();
    } else if(algoName == "roomsAndCorridors") {
        do {
            GenerateRooms();
            GenerateCorridors();
            _map->GetPathfinder()->Initialize(IntVector2{ _map->CalcMaxDimensions() });
        } while(!GenerateExitAndEntrance());
        PlaceActors();
        PlaceFeatures();
        PlaceItems();
    }
}

void MapGenerator::GenerateRandomRooms() noexcept {
    DataUtils::ValidateXmlElement(*_xml_element, "mapGenerator", "minSize,maxSize", "count,floor,wall,default", "", "down,up,enter,exit,width,height");
    const auto min_size = std::clamp([&]()->const int { const auto* xml_min = _xml_element->FirstChildElement("minSize"); int result = DataUtils::ParseXmlElementText(*xml_min, 1); if(result < 0) result = 1; return result; }(), 1, Map::max_dimension); //IIIL
    const auto max_size = std::clamp([&]()->const int { const auto* xml_max = _xml_element->FirstChildElement("maxSize"); int result = DataUtils::ParseXmlElementText(*xml_max, 1); if(result < 0) result = 1; return result; }(), 1, Map::max_dimension); //IIIL
    const int room_count = DataUtils::ParseXmlAttribute(*_xml_element, "count", 1);
    const int width = std::clamp(DataUtils::ParseXmlAttribute(*_xml_element, "width", 1), 1, Map::max_dimension);
    const int height = std::clamp(DataUtils::ParseXmlAttribute(*_xml_element, "height", 1), 1, Map::max_dimension);
    rooms.reserve(room_count);
    for(int i = 0; i < room_count; ++i) {
        const auto w = MathUtils::GetRandomInRange(min_size, max_size);
        const auto h = MathUtils::GetRandomInRange(min_size, max_size);
        const auto x = MathUtils::GetRandomInRange(w, width - (2 * w));
        const auto y = MathUtils::GetRandomInRange(h, height - (2 * h));
        rooms.push_back(AABB2{ Vector2{(float)x, (float)y}, (float)w, (float)h });
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
        world_bounds.StretchToIncludePoint(room.mins - Vector2::One);
        world_bounds.StretchToIncludePoint(room.maxs + Vector2::One);
    }
    const auto map_width = static_cast<int>(world_bounds.CalcDimensions().x);
    const auto map_height = static_cast<int>(world_bounds.CalcDimensions().y);
    if(_map->_layers.empty()) {
        _map->_layers.emplace_back(std::make_unique<Layer>(_map, IntVector2{ map_width, map_height }));
    } else {
        if(_map->_layers[0]->tileDimensions != IntVector2{ map_width, map_height }) {
            _map->_layers[0] = std::move(std::make_unique<Layer>(_map, IntVector2{ map_width, map_height }));
        }
    }
    floorType = DataUtils::ParseXmlAttribute(*_xml_element, "floor", floorType);
    wallType = DataUtils::ParseXmlAttribute(*_xml_element, "wall", wallType);
    defaultType = DataUtils::ParseXmlAttribute(*_xml_element, "default", defaultType);
    stairsDownType = DataUtils::ParseXmlAttribute(*_xml_element, "down", stairsDownType);
    stairsUpType = DataUtils::ParseXmlAttribute(*_xml_element, "up", stairsUpType);
    enterType = DataUtils::ParseXmlAttribute(*_xml_element, "enter", enterType);
    exitType = DataUtils::ParseXmlAttribute(*_xml_element, "exit", exitType);

    if(auto* layer = _map->GetLayer(0); layer != nullptr) {
        for(auto& tile : *layer) {
            tile.ChangeTypeFromName(defaultType);
        }
    }
    FillRoomsWithWallTiles();
    FillRoomsWithFloorTiles();
}

void MapGenerator::GenerateRooms() noexcept {
    //Ref: Using "floorplan" algorithm here: https://www.reddit.com/r/roguelikedev/comments/310ae2/looking_for_a_bit_of_help_on_a_dungeon_generator/cpxrfbh?utm_source=share&utm_medium=web2x&context=3
    //1.  Make up some general constraints, like maximum and minimum room width & height.
    //2.  You need some sort of generic "room" construct, abstract from the map.
    //    Rooms are basically just rectangles with a top (y), left (x), width, height.
    //    From there, it is simple to know what tiles are floor, walls, or corner walls.
    //3.  Place a single (random size) room somewhere on the map;
    //        add it to a list of rooms.
    //4.  Begin Loop while % of tiles in rooms is < threshold% of map:
    //5.      Pick a room out of your list of rooms (initialized in step 3);
    //            this is your "base" room, for this pass.
    //6.      Pick a random wall (non corner) of that base room.
    //7.      Make a "new" room, new random height, width (x, y will be determined in the next step).
    //8.      Place the new room such that it overlaps that wall/side of the base room;
    //        you can "slide" it up and down or left-to-right according to the new room's width.
    //        Make sure the walls actually overlap;
    //        you don't want double walls because that's ugly and makes doors look stupid.
    //9.      Check the corners of this new room;
    //        if it's outside of the map;
    //            chuck it out and start over at step 4.
    //10.     Check to see if the new room overlaps any existing rooms;
    //            if so, chuck it out and start over at step 4.
    //11.     Add the new room to the list of rooms;
    //            Make sure to keep track of the specific wall coordinate that you picked in step 6;
    //            this will be a candidate for a door.
    //12. End Loop;
    //until the map is filled up to X%.
    //Keep in mind the higher X%;
    //the longer the algorithm will take to find that one last tiny,
    //perfectly-shaped room to fit and bump you over the percentage requirement.

    DataUtils::ValidateXmlElement(*_xml_element, "mapGenerator", "minSize,maxSize", "floor,wall", "", "coverage,down,up,enter,exit,width,height");
    //Step 1.
    const auto max_tile_coverage = DataUtils::ParseXmlAttribute(*_xml_element, "coverage", 0.10f);
    GUARANTEE_OR_DIE(0.0f <= max_tile_coverage && max_tile_coverage <= 1.0f, "RoomsOnlyMapGenerator: coverage value out of [0.0, 1.0f] range.");
    const auto min_size = std::clamp([&]()->const int { const auto* xml_min = _xml_element->FirstChildElement("minSize"); int result = DataUtils::ParseXmlElementText(*xml_min, 1); if(result < 0) result = 1; return result; }(), 1, Map::max_dimension); //IIIL
    const auto max_size = std::clamp([&]()->const int { const auto* xml_max = _xml_element->FirstChildElement("maxSize"); int result = DataUtils::ParseXmlElementText(*xml_max, 1); if(result < 0) result = 1; return result; }(), 1, Map::max_dimension); //IIIL
    const int width = std::clamp(DataUtils::ParseXmlAttribute(*_xml_element, "width", 1), 1, Map::max_dimension);
    const int height = std::clamp(DataUtils::ParseXmlAttribute(*_xml_element, "height", 1), 1, Map::max_dimension);

    floorType = DataUtils::ParseXmlAttribute(*_xml_element, "floor", floorType);
    wallType = DataUtils::ParseXmlAttribute(*_xml_element, "wall", wallType);
    defaultType = DataUtils::ParseXmlAttribute(*_xml_element, "default", defaultType);
    stairsDownType = DataUtils::ParseXmlAttribute(*_xml_element, "down", stairsDownType);
    stairsUpType = DataUtils::ParseXmlAttribute(*_xml_element, "up", stairsUpType);
    enterType = DataUtils::ParseXmlAttribute(*_xml_element, "enter", enterType);
    exitType = DataUtils::ParseXmlAttribute(*_xml_element, "exit", exitType);


    defaultType = wallType;

    if(_map->_layers.empty()) {
        _map->_layers.emplace_back(std::make_unique<Layer>(_map, IntVector2{ width, height }));
    } else {
        if(_map->_layers[0]->tileDimensions != IntVector2{ width, height }) {
            _map->_layers[0] = std::move(std::make_unique<Layer>(_map, IntVector2{ width, height }));
        }
    }

    for(auto& tile : *_map->GetLayer(0)) {
        tile.ChangeTypeFromName(defaultType);
    }
    //Step 2.
    //Step 3.
    {
        const auto w = MathUtils::GetRandomInRange(min_size, max_size);
        const auto h = MathUtils::GetRandomInRange(min_size, max_size);
        const auto x = MathUtils::GetRandomLessThan(width);
        const auto y = MathUtils::GetRandomLessThan(height);
        rooms.push_back(AABB2{ (float)x, (float)y, (float)x + (float)w, (float)y + +(float)h });
    }
    const auto calcTileCoverage = [&]() {
        int count{ 0 };
        for(const auto& room : rooms) {
            count += static_cast<int>(_map->GetTilesInArea(room).size());
        }
        const auto area = width * height;
        return count / static_cast<float>(area);
    };
    //Step 4.
    while(calcTileCoverage() < max_tile_coverage) {
        //Step 5.
        const auto base_room_index = MathUtils::GetRandomLessThan(rooms.size());
        auto& base_room = rooms[base_room_index];
        //Step 6.
        const auto new_room_position_offset = [&]() {
            const auto base_room_center = IntVector2(base_room.CalcCenter());
            const auto base_room_half_extents = IntVector2(base_room.CalcDimensions()) / 2;
            auto result = IntVector2{};
            switch(MathUtils::GetRandomLessThan(4)) {
            case 0: { result.x = base_room_center.x - base_room_half_extents.x; break; } //West wall
            case 1: { result.x = base_room_center.x + base_room_half_extents.x; break; } //East wall
            case 2: { result.y = base_room_center.y - base_room_half_extents.y; break; } //North wall
            case 3: { result.y = base_room_center.y + base_room_half_extents.y; break; } //South wall
            }
            return result;
        }();
        //Step 7.
        auto new_room = AABB2{};
        const auto w = MathUtils::GetRandomInRange(min_size, max_size);
        const auto h = MathUtils::GetRandomInRange(min_size, max_size);
        new_room.maxs = Vector2{ static_cast<float>(w), static_cast<float>(h) };
        //Step 8.
        auto new_position = new_room_position_offset;
        if(new_room_position_offset.x != 0) {
            new_position.x += (w / 2) * (new_room_position_offset.x < 0 ? -1 : 1);
            new_position.y += MathUtils::GetRandomLessThan((h / 2));
        } else if(new_room_position_offset.y != 0) {
            new_position.y += (h / 2) * (new_room_position_offset.y < 0 ? -1 : 1);
            new_position.x += MathUtils::GetRandomLessThan((w / 2));
        }
        new_room.Translate(Vector2(new_position));
        //Step 9.
        const auto world_bounds = AABB2{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height) };
        if(!MathUtils::Contains(world_bounds, new_room)) {
            continue;
        }
        //Step 10.
        if(const auto rooms_overlap = [&]() {
            for(const auto& room : rooms) {
                if(MathUtils::DoAABBsOverlap(new_room, room)) {
                    return true;
                }
            }
            return false;
        }()) //IIIL
        {
            continue;
        }
            rooms.push_back(new_room);
            doors.push_back(new_room_position_offset);
    }
    for(auto& door : doors) {
        if(auto* tile = _map->GetTile(door.x, door.y, 0); tile != nullptr) {
            tile->ChangeTypeFromName("floor");
        }
        //TODO: Feature Instancing!! (Doors).
    }
    FillRoomsWithFloorTiles();

    _map->GetPathfinder()->Initialize(IntVector2{ _map->CalcMaxDimensions() });
    LoadFeatures(*_map->_root_xml_element);
    LoadActors(*_map->_root_xml_element);
    LoadItems(*_map->_root_xml_element);
    PlaceActors();
    PlaceFeatures();
    PlaceItems();
}

void MapGenerator::FillAreaWithTileType(const AABB2& area, std::string typeName) noexcept {
    for(auto* tile : _map->GetTilesInArea(area)) {
        tile->ChangeTypeFromName(typeName);
    }

}
void MapGenerator::FillRoomsWithTileType(std::string typeName) noexcept {
    for(const auto& room : rooms) {
        FillAreaWithTileType(room, typeName);
    }
}


void MapGenerator::FillRoomsWithWallTiles() noexcept {
    FillRoomsWithTileType(wallType);
}

void MapGenerator::FillRoomsWithFloorTiles() noexcept {
    FillRoomsWithTileType(floorType);
}

void MapGenerator::GenerateCorridors() noexcept {
    const auto roomCount = rooms.size();
    for(auto i = std::size_t{ 0u }; i != roomCount; ++i) {
        const auto& r1 = rooms[i % roomCount];
        const auto& r2 = rooms[(i + 1u) % roomCount];
        const auto horizontal_first = MathUtils::GetRandomBool();
        if(horizontal_first) {
            MakeHorizontalCorridor(r1, r2);
            MakeVerticalCorridor(r2, r1);
        } else {
            MakeVerticalCorridor(r1, r2);
            MakeHorizontalCorridor(r2, r1);
        }
    }
    FillRoomsWithFloorTiles();
}

void MapGenerator::MakeVerticalCorridor(const AABB2& from, const AABB2& to) noexcept {
    const auto [start, end] = [&]() {
        auto start = from.CalcCenter().y;
        auto end = to.CalcCenter().y;
        if(end < start) {
            std::swap(end, start);
        }
        return std::make_pair(start, end);
    }(); //IIIL
    const auto step = std::signbit(end - start) ? -1.0f : 1.0f;
    const auto x = from.CalcCenter().x;
    for(auto y = start; y <= end; y += step) {
        MakeCorridorSegmentAt(x, y);
    }
}

void MapGenerator::MakeHorizontalCorridor(const AABB2& from, const AABB2& to) noexcept {
    const auto [start, end] = [&]() {
        auto start = from.CalcCenter().x;
        auto end = to.CalcCenter().x;
        if(end < start) {
            std::swap(end, start);
        }
        return std::make_pair(start, end);
    }(); //IIIL
    const auto step = std::signbit(end - start) ? -1.0f : 1.0f;
    const auto y = from.CalcCenter().y;
    for(auto x = start; x <= end; x += step) {
        MakeCorridorSegmentAt(x, y);
    }
}

void MapGenerator::MakeCorridorSegmentAt(float x, const float y) const noexcept {
    if(auto* tile = _map->GetTile(IntVector3{static_cast<int>(x), static_cast<int>(y), 0})) {
        tile->ChangeTypeFromName(floorType);
        const auto neighbors = tile->GetNeighbors();
        for(auto* neighbor : neighbors) {
            if(neighbor && CanTileBeCorridorWall(neighbor->GetType())) {
                neighbor->ChangeTypeFromName(wallType);
            }
        }
    }
}

bool MapGenerator::VerifyExitIsReachable(const IntVector2& enter_loc, const IntVector2& exit_loc) const noexcept {
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

bool MapGenerator::CanTileBeCorridorWall(const std::string& name) const noexcept {
    return name != this->floorType;
}

bool MapGenerator::GenerateExitAndEntrance() noexcept {
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
                const auto room_id_with_down = MathUtils::GetRandomLessThan(roomCountAsInt);
                const auto room_id_with_up = [roomCountAsInt, room_id_with_down]()->int {
                    auto result = MathUtils::GetRandomLessThan(roomCountAsInt);
                    while(result == room_id_with_down) {
                        result = MathUtils::GetRandomLessThan(roomCountAsInt);
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
        const auto exit_loc = IntVector2{room_with_exit.mins + Vector2::One};
        const auto enter_loc = IntVector2{room_with_entrance.mins + Vector2::One};
        start = enter_loc;
        end = exit_loc;
    } while(!VerifyExitIsReachable(start, end));

    if(auto* tile = _map->GetTile(IntVector3{start, 0})) {
        tile->ChangeTypeFromName(stairsUpType);
        tile->SetEntrance();
    }
    if(auto* tile = _map->GetTile(IntVector3{end, 0})) {
        tile->ChangeTypeFromName(stairsDownType);
        tile->SetExit();
    }
    return true;
}
