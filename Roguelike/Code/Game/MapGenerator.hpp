#pragma once

#include "Engine/Core/DataUtils.hpp"

#include "Engine/Math/AABB2.hpp"

#include <string>
#include <vector>

class Map;

class MapGenerator {
public:
    MapGenerator() = delete;
    explicit MapGenerator(Map* map, const XMLElement& elem) noexcept;
    MapGenerator(const MapGenerator& other) = delete;
    MapGenerator(MapGenerator&& other) = delete;
    MapGenerator& operator=(const MapGenerator& other) = delete;
    MapGenerator& operator=(MapGenerator&& other) = delete;
    virtual ~MapGenerator() noexcept = default;
    virtual void Generate() = 0;
protected:
    void LoadLayers(const XMLElement& elem);

    const XMLElement& _xml_element;
    Map* _map = nullptr;
private:
};

class HeightMapGenerator : public MapGenerator {
public:
    HeightMapGenerator() = delete;
    explicit HeightMapGenerator(Map* map, const XMLElement& elem) noexcept;
    HeightMapGenerator(const HeightMapGenerator& other) = delete;
    HeightMapGenerator(HeightMapGenerator&& other) = delete;
    HeightMapGenerator& operator=(const HeightMapGenerator& other) = delete;
    HeightMapGenerator& operator=(HeightMapGenerator&& other) = delete;
    virtual ~HeightMapGenerator() noexcept = default;
    void Generate() override;
protected:
private:
};

class FileMapGenerator : public MapGenerator {
public:
    FileMapGenerator() = delete;
    explicit FileMapGenerator(Map* map, const XMLElement& elem) noexcept;
    FileMapGenerator(const FileMapGenerator& other) = delete;
    FileMapGenerator(FileMapGenerator&& other) = delete;
    FileMapGenerator& operator=(const FileMapGenerator& other) = delete;
    FileMapGenerator& operator=(FileMapGenerator&& other) = delete;
    virtual ~FileMapGenerator() noexcept = default;
    void Generate() override;
protected:
private:
    void LoadLayersFromFile(const XMLElement& elem);
};

class XmlMapGenerator : public MapGenerator {
public:
    XmlMapGenerator() = delete;
    explicit XmlMapGenerator(Map* map, const XMLElement& elem) noexcept;
    XmlMapGenerator(const XmlMapGenerator& other) = delete;
    XmlMapGenerator(XmlMapGenerator&& other) = delete;
    XmlMapGenerator& operator=(const XmlMapGenerator& other) = delete;
    XmlMapGenerator& operator=(XmlMapGenerator&& other) = delete;
    virtual ~XmlMapGenerator() noexcept = default;
    void Generate() override;
protected:
private:
    void LoadLayersFromXml(const XMLElement& elem);
};

class MazeMapGenerator : public MapGenerator {
public:
    MazeMapGenerator() = delete;
    explicit MazeMapGenerator(Map* map, const XMLElement& elem) noexcept;
    MazeMapGenerator(const MazeMapGenerator& other) = delete;
    MazeMapGenerator(MazeMapGenerator&& other) = delete;
    MazeMapGenerator& operator=(const MazeMapGenerator& other) = delete;
    MazeMapGenerator& operator=(MazeMapGenerator&& other) = delete;
    virtual ~MazeMapGenerator() noexcept = default;
    static void Generate(Map* map, const XMLElement& elem);
    virtual void Generate() = 0;
protected:
private:
};

class RoomsMapGenerator : public MazeMapGenerator {
public:
    RoomsMapGenerator() = delete;
    explicit RoomsMapGenerator(Map* map, const XMLElement& elem) noexcept;
    RoomsMapGenerator(const RoomsMapGenerator& other) = delete;
    RoomsMapGenerator(RoomsMapGenerator&& other) = delete;
    RoomsMapGenerator& operator=(const RoomsMapGenerator& other) = delete;
    RoomsMapGenerator& operator=(RoomsMapGenerator&& other) = delete;
    virtual ~RoomsMapGenerator() noexcept = default;
    void Generate() override;
protected:
    std::string defaultType{"void"};
    std::string floorType{"void"};
    std::string wallType{"void"};
    std::string stairsDownType{"void"};
    std::string stairsUpType{"void"};
    std::string enterType{"void"};
    std::string exitType{"void"};
    std::vector<AABB2> rooms{};
private:
};

class RoomsAndCorridorsMapGenerator : public RoomsMapGenerator {
public:
    RoomsAndCorridorsMapGenerator() = delete;
    explicit RoomsAndCorridorsMapGenerator(Map* map, const XMLElement& elem) noexcept;
    RoomsAndCorridorsMapGenerator(const RoomsAndCorridorsMapGenerator& other) = delete;
    RoomsAndCorridorsMapGenerator(RoomsAndCorridorsMapGenerator&& other) = delete;
    RoomsAndCorridorsMapGenerator& operator=(const RoomsAndCorridorsMapGenerator& other) = delete;
    RoomsAndCorridorsMapGenerator& operator=(RoomsAndCorridorsMapGenerator&& other) = delete;
    virtual ~RoomsAndCorridorsMapGenerator() noexcept = default;
    void Generate() override;

private:
    void GenerateCorridors() noexcept;
    void MakeHorizontalCorridor(const AABB2& r1, const AABB2& r2) noexcept;
    void MakeVerticalCorridor(const AABB2& r1, const AABB2& r2) noexcept;
};
