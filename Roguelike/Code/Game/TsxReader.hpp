#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/FileUtils.hpp"

#include "Engine/Profiling/Instrumentor.hpp"

#include <filesystem>
#include <string>

struct TsxDesc {
    std::filesystem::path filepath;
    std::string name;
    int firstGid{ 1 };
    int tileWidth{ 1 };
    int tileHeight{ 1 };
    int tileCount{ 1 };
    int columnCount{ 1 };
    int tileoffset{ 0 };

};

class TsxReader {
public:
    TsxReader() noexcept = default;
    TsxReader(const TsxReader& other) noexcept = default;
    TsxReader(TsxReader&& other) noexcept = default;
    TsxReader& operator=(const TsxReader& other) noexcept = default;
    TsxReader& operator=(TsxReader&& other) noexcept = default;
    ~TsxReader() noexcept = default;

    explicit TsxReader(std::filesystem::path filepath) noexcept;
    
    bool LoadFile(std::filesystem::path filepath) noexcept;
    void Parse() noexcept;

    TsxDesc description{};

protected:
private:
    bool LoadFile() noexcept;

    void LoadTmxTileset(const XMLElement& elem) noexcept;

    tinyxml2::XMLDocument m_xmlDoc;
    XMLElement* m_rootElement{ nullptr };
};
