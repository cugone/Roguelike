#include "Game/MapGenerator.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Game/Actor.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

MapGenerator::MapGenerator(Map* map, const XMLElement& elem) noexcept
    : _xml_element(elem)
    , _map(map)
{ /* DO NOTHING */
}

void MapGenerator::LoadLayers(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "layers", "layer", "");
    std::size_t layer_count = DataUtils::GetChildElementCount(elem, "layer");
    if(layer_count > _map->max_layers) {
        const auto ss = std::string{"Layer count of map "} +_map->_name + " is greater than the maximum allowed (" + std::to_string(_map->max_layers) + ")."
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
                if(t.color.r < glyph_height) {
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

XmlMapGenerator::XmlMapGenerator(Map* map, const XMLElement& elem) noexcept
    : MapGenerator(map, elem)
{
    /* DO NOTHING */
}

void XmlMapGenerator::Generate() {
    DataUtils::ValidateXmlElement(_xml_element, "mapGenerator", "layers", "");
    LoadLayersFromXml(_xml_element);
}

void XmlMapGenerator::LoadLayersFromXml(const XMLElement& elem) {
    if(auto xml_layers = elem.FirstChildElement("layers")) {
        LoadLayers(*xml_layers);
    }
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
        RoomsMapGenerator g{map, elem};
        g.Generate();
    } else if(algoName == "roomsAndCorridors") {
        RoomsAndCorridorsMapGenerator g{map,elem};
        g.Generate();
    }
}

RoomsMapGenerator::RoomsMapGenerator(Map* map, const XMLElement& elem) noexcept
: MazeMapGenerator(map, elem)
{
    /* DO NOTHING */
}

void RoomsMapGenerator::Generate() {
    DataUtils::ValidateXmlElement(_xml_element, "mapGenerator", "minSize,maxSize", "width,height,count,floor,wall,default", "", "");
    const int width = DataUtils::ParseXmlAttribute(_xml_element, "width", 1);
    const int height = DataUtils::ParseXmlAttribute(_xml_element, "height", 1);
    _map->_layers.emplace_back(std::make_unique<Layer>(_map, IntVector2{width, height}));
    const int room_count = DataUtils::ParseXmlAttribute(_xml_element, "count", 1);
    floorType = DataUtils::ParseXmlAttribute(_xml_element, "floor", floorType);
    wallType = DataUtils::ParseXmlAttribute(_xml_element, "wall", wallType);
    defaultType = DataUtils::ParseXmlAttribute(_xml_element, "default", defaultType);
    stairsDownType = DataUtils::ParseXmlAttribute(_xml_element, "stairsDown", stairsDownType);
    stairsUpType = DataUtils::ParseXmlAttribute(_xml_element, "stairsUp", stairsUpType);
    enterType = DataUtils::ParseXmlAttribute(_xml_element, "enter", enterType);
    exitType = DataUtils::ParseXmlAttribute(_xml_element, "exit", exitType);
    {
        auto* layer = _map->GetLayer(0);
        for(auto& tile : *layer) {
            tile.ChangeTypeFromName(defaultType);
        }
    }
    const auto* xml_min = _xml_element.FirstChildElement("minSize");
    const auto min_size = DataUtils::ParseXmlElementText(*xml_min, 1);
    const auto* xml_max = _xml_element.FirstChildElement("maxSize");
    const auto max_size = DataUtils::ParseXmlElementText(*xml_max, 1);

    for(int i = 0; i < room_count; ++i) {
        const auto random_room_size = MathUtils::GetRandomIntInRange(min_size, max_size);
        const auto roomSizeAsFloat = static_cast<float>(random_room_size);
        const auto valid_room_bounds = [&]()->const AABB2 { AABB2 bounds = _map->CalcWorldBounds(); bounds.AddPaddingToSides(-roomSizeAsFloat, -roomSizeAsFloat); return bounds; }(); //IIIL
        const auto room_position = IntVector2{MathUtils::GetRandomPointInside(valid_room_bounds)};
        rooms.push_back(std::make_pair(room_position, random_room_size));
        if(const auto* room_center_tile = _map->GetTile(IntVector3{room_position, 0})) {
            const auto roomFloorTiles = _map->GetTilesWithinDistance(*room_center_tile, roomSizeAsFloat, [&](const IntVector2& start, const IntVector2& end) { return MathUtils::CalculateChessboardDistance(start, end); });
            for(auto& tile : roomFloorTiles) {
                tile->ChangeTypeFromName(floorType);
            }
            const auto roomWallTiles = _map->GetTilesAtDistance(*room_center_tile, roomSizeAsFloat, [&](const IntVector2& start, const IntVector2& end) { return MathUtils::CalculateChessboardDistance(start, end); });
            for(auto& tile : roomWallTiles) {
                tile->ChangeTypeFromName(wallType);
            }
        }
    }
    _map->_player_start_position = rooms[0].first - IntVector2{rooms[0].second, rooms[0].second};
}

RoomsAndCorridorsMapGenerator::RoomsAndCorridorsMapGenerator(Map* map, const XMLElement& elem) noexcept
    : RoomsMapGenerator(map, elem)
{
    /* DO NOTHING */
}

void RoomsAndCorridorsMapGenerator::Generate() {
    RoomsMapGenerator::Generate();
    const auto roomCount = rooms.size();
    for(int i = 0; i <= roomCount; ++i) {
        const auto r1 = rooms[i % roomCount];
        const auto r1pos = r1.first;
        const auto r1size = r1.second;
        const auto r2 = rooms[(i + 1) % roomCount];
        const auto r2pos = r2.first;
        const auto r2size = r2.second;
        const auto [project, reject] = MathUtils::DivideIntoProjectAndReject(Vector2{r2.first - r1.first}, Vector2::X_AXIS);
        const auto projectDir = project.GetNormalize();
        const auto rejectDir = reject.GetNormalize();
        for(int x = r1pos.x; x != r2pos.x; x += (int)projectDir.x) {
            auto* tile = _map->GetTile(IntVector3{x, r1pos.y, 0});
            const auto tilebounds = tile->GetBounds();
            const bool corridor_candidate = !MathUtils::Contains(AABB2{Vector2{r1pos}, (float)r1size, (float)r1size}, tilebounds) && !MathUtils::Contains(AABB2{Vector2{r2pos}, (float)r2size, (float)r2size}, tilebounds);
            tile->ChangeTypeFromName(floorType);
            if(corridor_candidate) {
                auto* northtile = tile->GetNorthNeighbor();
                northtile->ChangeTypeFromName(wallType);
                auto* southtile = tile->GetSouthNeighbor();
                southtile->ChangeTypeFromName(wallType);
            }
        }
        for(int y = r1pos.y; y < r2pos.y; y += (int)rejectDir.y) {
            auto* tile = _map->GetTile(IntVector3{r2pos.x, y, 0});
            const auto tilebounds = tile->GetBounds();
            const bool corridor_candidate = !MathUtils::Contains(AABB2{Vector2{r1pos}, (float)r1size, (float)r1size}, tilebounds) && !MathUtils::Contains(AABB2{Vector2{r2pos}, (float)r2size, (float)r2size}, tilebounds);
            tile->ChangeTypeFromName(floorType);
            if(corridor_candidate) {
                auto* easttile = tile->GetEastNeighbor();
                easttile->ChangeTypeFromName(wallType);
                auto* westtile = tile->GetWestNeighbor();
                westtile->ChangeTypeFromName(wallType);
            }
        }
    }
    const int exitRoom = MathUtils::GetRandomIntLessThan((int)roomCount);
    auto* stairsDown_tile = _map->GetTile(IntVector3{rooms[exitRoom].first - IntVector2{rooms[exitRoom].second,rooms[exitRoom].second}, 0});
    stairsDown_tile->ChangeTypeFromName(stairsDownType);
}
