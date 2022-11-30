#pragma once

#include "Engine/Core/DataUtils.hpp"

#include "Engine/Math/AABB2.hpp"

#include <memory>
#include <string>
#include <vector>

class Map;

class MapGenerator {
public:
    MapGenerator() = default;
    explicit MapGenerator(Map* map, XMLElement* elem) noexcept;
    MapGenerator(const MapGenerator& other) = default;
    MapGenerator(MapGenerator&& other) = default;
    MapGenerator& operator=(const MapGenerator& other) = default;
    MapGenerator& operator=(MapGenerator&& other) = default;
    ~MapGenerator() noexcept = default;

    void SetRootXmlElement(const XMLElement& root_element) noexcept;
    void SetParentMap(Map* map) noexcept;
    void Generate() noexcept;

    std::vector<AABB2> rooms{};

protected:
private:
    void LoadLayers(const XMLElement& elem);

    void GenerateFromHeightMap() noexcept;
    void GenerateFromFile() noexcept;
    void GenerateMaze() noexcept;
    void GenerateRandomRooms() noexcept;
    void GenerateRooms() noexcept;
    void GenerateCorridors() noexcept;
    void FillAreaWithTileType(const AABB2& area, std::string typeName) noexcept;
    void FillRoomsWithTileType(std::string typeName) noexcept;
    void FillRoomsWithWallTiles() noexcept;
    void FillRoomsWithFloorTiles() noexcept;

    bool GenerateExitAndEntrance() noexcept;
    void MakeHorizontalCorridor(const AABB2& from, const AABB2& to) noexcept;
    void MakeCorridorSegmentAt(float x, const  float y) const noexcept;
    void MakeVerticalCorridor(const AABB2& from, const AABB2& to) noexcept;
    bool VerifyExitIsReachable(const IntVector2& enter_loc, const  IntVector2& exit_loc) const noexcept;
    bool CanTileBeCorridorWall(const std::string& name) const noexcept;

    void LoadItems(const XMLElement& elem) noexcept;
    void LoadActors(const XMLElement& elem) noexcept;
    void LoadFeatures(const XMLElement& elem) noexcept;
    void PlaceActors() noexcept;
    void PlaceFeatures() noexcept;
    void PlaceItems() noexcept;

    XMLElement* _xml_element{nullptr};
    Map* _map{nullptr};

    std::vector<IntVector2> doors{};

    std::string defaultType{ "void" };
    std::string floorType{ "void" };
    std::string wallType{ "void" };
    std::string stairsDownType{ "void" };
    std::string stairsUpType{ "void" };
    std::string enterType{ "void" };
    std::string exitType{ "void" };

};
