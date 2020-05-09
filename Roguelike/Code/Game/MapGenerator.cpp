#include "Game/MapGenerator.hpp"

#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Image.hpp"

#include "Engine/Math/MathUtils.hpp"

#include "Game/Actor.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Map.hpp"
#include "Game/TileDefinition.hpp"

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
    stairsDownType = DataUtils::ParseXmlAttribute(_xml_element, "down", stairsDownType);
    stairsUpType = DataUtils::ParseXmlAttribute(_xml_element, "up", stairsUpType);
    enterType = DataUtils::ParseXmlAttribute(_xml_element, "enter", enterType);
    exitType = DataUtils::ParseXmlAttribute(_xml_element, "exit", exitType);
    {
        auto* layer = _map->GetLayer(0);
        for(auto& tile : *layer) {
            tile.ChangeTypeFromName(defaultType);
        }
    }
    const auto min_size = [&]()->const int { const auto* xml_min = _xml_element.FirstChildElement("minSize"); int result = DataUtils::ParseXmlElementText(*xml_min, 0); if(result < 0) result = 0; return result; }(); //IIIL
    const auto max_size = [&]()->const int { const auto* xml_max = _xml_element.FirstChildElement("maxSize"); int result = DataUtils::ParseXmlElementText(*xml_max, 0); if(result < 0) result = 0; return result; }(); //IIIL
    rooms.reserve(room_count);
    unsigned int remake_attempts = 0;
    for(int i = 0; i < room_count; ++i) {
        if(remake_attempts > 10) {
            remake_attempts = 0;
            break;
        }
        const auto random_room_size = Vector2{IntVector2{MathUtils::GetRandomIntInRange(min_size, max_size), MathUtils::GetRandomIntInRange(min_size, max_size)}};
        const auto roomWidthAsFloat = static_cast<float>(random_room_size.x);
        const auto roomHeightAsFloat = static_cast<float>(random_room_size.y);
        const auto valid_room_bounds = [&]()->const AABB2 { AABB2 bounds = _map->CalcWorldBounds(); bounds.AddPaddingToSides(-(roomWidthAsFloat + 1.0f), -(roomHeightAsFloat + 1.0f)); return bounds; }(); //IIIL
        bool rooms_overlap = false;
        bool room_doesnt_fit = false;
        bool room_needs_repositioning = false;
        unsigned int reposition_attempts = 0;
        AABB2 room_bounds{};
        do {
            if(reposition_attempts > 1000) {
                break;
            }
            rooms_overlap = false;
            room_doesnt_fit = false;
            const auto room_position = IntVector2{MathUtils::GetRandomPointInside(valid_room_bounds)};
            const auto room_with_walls_bounds = AABB2{Vector2{room_position}, random_room_size.x + 1, random_room_size.y + 1};
            for(const auto& room : rooms) {
                if(MathUtils::DoAABBsOverlap(room_with_walls_bounds, room)) {
                    rooms_overlap = true;
                    break;
                }
                if(!MathUtils::Contains(valid_room_bounds, room_with_walls_bounds)) {
                    room_doesnt_fit = true;
                    break;
                }
            }
            room_needs_repositioning = rooms_overlap || room_doesnt_fit;
            if(room_needs_repositioning) {
                ++reposition_attempts;
                continue;
            }
            room_bounds = room_with_walls_bounds;
        } while(room_needs_repositioning);
        if(reposition_attempts > 1000) {
            reposition_attempts = 0;
            --i;
            ++remake_attempts;
            continue;
        }
        remake_attempts = 0;
        rooms.push_back(room_bounds);
        const auto roomWallTiles = _map->GetTilesInArea(room_bounds);
        for(auto& tile : roomWallTiles) {
            tile->ChangeTypeFromName(wallType);
        }
        const auto room_floor_bounds = [&]() { auto bounds = room_bounds; bounds.AddPaddingToSides(-1.0f, -1.0f); return bounds; }();
        const auto roomFloorTiles = _map->GetTilesInArea(room_floor_bounds);
        for(auto& tile : roomFloorTiles) {
            tile->ChangeTypeFromName(floorType);
        }
    }
}

RoomsAndCorridorsMapGenerator::RoomsAndCorridorsMapGenerator(Map* map, const XMLElement& elem) noexcept
    : RoomsMapGenerator(map, elem)
{
    /* DO NOTHING */
}

void RoomsAndCorridorsMapGenerator::Generate() {
    RoomsMapGenerator::Generate();
    GenerateCorridors();
    // TODO: Generate exit
}

void RoomsAndCorridorsMapGenerator::GenerateCorridors() noexcept {
    const auto roomCount = rooms.size();
    for(int i = 0; i < roomCount - 1; ++i) {
        const auto r1 = rooms[i];
        const auto r2 = rooms[i + 1];
        const auto r1pos = r1.CalcCenter();
        const auto r1dims = r1.CalcDimensions();
        const auto r2pos = r2.CalcCenter();
        const auto r2dims = r2.CalcDimensions();
        const auto horizontal_first = MathUtils::GetRandomBool();
        if(horizontal_first) {
            MakeHorizontalCorridor(r1, r2);
            MakeVerticalCorridor(r1, r2);
        } else {
            MakeVerticalCorridor(r1, r2);
            MakeHorizontalCorridor(r1, r2);
        }
    }
}

void RoomsAndCorridorsMapGenerator::MakeVerticalCorridor(const AABB2& r1, const AABB2& r2) noexcept {
    const bool start_at_r1 = MathUtils::GetRandomBool();
    auto start = (start_at_r1 ? r1.CalcCenter() : r2.CalcCenter()).y;
    auto end = (start_at_r1 ? r2.CalcCenter() : r1.CalcCenter()).y;
    if(end < start) {
        std::swap(end, start);
    }
    const auto step = std::signbit(end - start) ? -1.0f : 1.0f;
    const auto x = (start_at_r1 ? r1.CalcCenter() : r2.CalcCenter()).x;
    const auto can_be_corridor_wall = [&](const std::string& name) { return name != this->floorType; };
    for(auto y = start; y < end; y += step) {
        auto* tile = _map->GetTile(IntVector3{static_cast<int>(x), static_cast<int>(y), 0});
        tile->ChangeTypeFromName(floorType);
        const auto neighbors = tile->GetNeighbors();
        for(auto* neighbor : neighbors) {
            if(neighbor && can_be_corridor_wall(neighbor->GetDefinition()->name)) {
                neighbor->ChangeTypeFromName(wallType);
            }
        }
    }
}

void RoomsAndCorridorsMapGenerator::MakeHorizontalCorridor(const AABB2& r1, const AABB2& r2) noexcept {
    const bool start_at_r1 = MathUtils::GetRandomBool();
    auto start = (start_at_r1 ? r1.CalcCenter() : r2.CalcCenter()).x;
    auto end = (start_at_r1 ? r2.CalcCenter() : r1.CalcCenter()).x;
    if(end < start) {
        std::swap(end, start);
    }
    const auto step = std::signbit(end - start) ? -1.0f : 1.0f;
    const auto y = (start_at_r1 ? r1.CalcCenter() : r2.CalcCenter()).y;
    const auto can_be_corridor_wall = [&](const std::string& name) { return name != this->floorType; };
    for(auto x = start; x < end; x += step) {
        auto* tile = _map->GetTile(IntVector3{static_cast<int>(x), static_cast<int>(y), 0});
        tile->ChangeTypeFromName(floorType);
        const auto neighbors = tile->GetNeighbors();
        for(auto* neighbor : neighbors) {
            if(neighbor && can_be_corridor_wall(neighbor->GetDefinition()->name)) {
                neighbor->ChangeTypeFromName(wallType);
            }
        }
    }
}
