#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <filesystem>
#include <memory>
#include <utility>

class Map;
class Layer;
struct TsxDesc;

struct TmxReaderDesc {
    std::string exportTarget{};
    std::string exportFormat{};
    uint16_t chunkWidth{ 1u };
    uint16_t chunkHeight{ 1u };
};

class TmxReader {
public:
    TmxReader() noexcept = default;
    TmxReader(const TmxReader& other) noexcept = default;
    TmxReader(TmxReader&& other) noexcept = default;
    TmxReader& operator=(const TmxReader& other) noexcept = default;
    TmxReader& operator=(TmxReader&& other) noexcept = default;
    ~TmxReader() noexcept = default;

    explicit TmxReader(std::filesystem::path filepath) noexcept;

    bool LoadFile(std::filesystem::path filepath) noexcept;
    void Parse(Map& map) noexcept;

    TmxReaderDesc description{};

protected:
private:
    bool LoadFile() noexcept;

    std::pair<int, std::filesystem::path> ParseTilesetElement(const XMLElement& elem) noexcept;
    void ParseLayerElements(Map& map, const XMLElement& elem, const TsxDesc& tsxDescription) noexcept;
    void InitializeTilesFromTmxData(Layer* layer, const XMLElement& elem, int firstgid) noexcept;

    std::filesystem::path m_filepath{};
    tinyxml2::XMLDocument m_xmlDoc;
    tinyxml2::XMLElement* m_rootXml{ nullptr };
};
