#include "Game/TmxReader.hpp"

#include "Engine/Core/Base64.hpp"
#include "Engine/Core/DataUtils.hpp"

#include "Game/Map.hpp"
#include "Game/Layer.hpp"
#include "Game/TsxReader.hpp"

#include <Thirdparty/TinyXML2/tinyxml2.h>

TmxReader::TmxReader(std::filesystem::path filepath) noexcept
    : m_filepath{filepath}
{
    GUARANTEE_OR_DIE(LoadFile(), "Failed to load TMX map file.");
}

bool TmxReader::LoadFile() noexcept {
    return LoadFile(m_filepath);
}

bool TmxReader::LoadFile(std::filesystem::path filepath) noexcept {
    if(!FileUtils::IsSafeReadPath(filepath)) {
        DebuggerPrintf(std::format("WARNING: TMX map file \"{:s}\" could not be parsed.\n", filepath));
        return false;
    }
    filepath = std::filesystem::canonical(filepath);
    m_filepath = filepath;
    m_rootXml = nullptr;
    if(auto xml_result = m_xmlDoc.LoadFile(m_filepath.string().c_str()); xml_result == tinyxml2::XML_SUCCESS) {
        m_rootXml = m_xmlDoc.RootElement();
        return true;
    } else {
        DebuggerPrintf(std::format("WARNING: TMX map file \"{:s}\" could not be loaded. XML parser returned: {:s}", m_filepath, m_xmlDoc.ErrorStr()));
        return false;
    }
}

void TmxReader::Parse(Map& map) noexcept {
    if(m_rootXml == nullptr) {
        GUARANTEE_OR_DIE(LoadFile(m_filepath), "Failed to load TMX map file.");
    }

    DataUtils::ValidateXmlElement(*m_rootXml, "map", "tileset", "version,orientation,width,height,tilewidth,tileheight", "properties,editorsettings,layer,objectgroup,imagelayer,group", "tiledversion,class,renderorder,compressionlevel,parallaxoriginx,parallaxoriginy,backgroundcolor,nextlayerid,nextobjectid,infinite,hexsidelength,staggeraxis,staggerindex");

    {
        const auto verify_version = [](const XMLElement& elem, std::string versionAttributeName, const std::string requiredVersionString) {
            if(const auto version_string = DataUtils::ParseXmlAttribute(elem, versionAttributeName, std::string{ "0.0" }); version_string != requiredVersionString) {
                const auto required_versions = StringUtils::Split(requiredVersionString, '.', false);
                const auto actual_versions = StringUtils::Split(version_string, '.', false);
                const auto required_major_version = std::stoi(required_versions[0]);
                const auto required_minor_version = std::stoi(required_versions[1]);
                const auto actual_major_version = std::stoi(actual_versions[0]);
                const auto actual_minor_version = std::stoi(actual_versions[1]);
                if(actual_major_version < required_major_version || (actual_major_version == required_major_version && actual_minor_version < required_minor_version)) {
                    const auto msg = std::format("ERROR: Attribute mismatch for \"{}\". Required: {} File: {}\n", versionAttributeName, requiredVersionString, version_string);
                    ERROR_AND_DIE(msg.c_str());
                }
            }
        };

        constexpr auto required_tmx_version = "1.9";
        verify_version(*m_rootXml, "version", required_tmx_version);

        constexpr auto required_tiled_version = "1.9.2";
        verify_version(*m_rootXml, "tiledversion", required_tiled_version);
    }

    if(const auto prop_count = DataUtils::GetChildElementCount(*m_rootXml, "properties"); prop_count > 1) {
        DebuggerPrintf(std::format("WARNING: TMX map file map element contains more than one \"properties\" element. Ignoring all after first.\n"));
    }
    if(const auto editorsettings_count = DataUtils::GetChildElementCount(*m_rootXml, "editorsettings"); editorsettings_count > 1) {
        DebuggerPrintf(std::format("WARNING: TMX map file map element contains more than one \"editorsettings\" element. Ignoring all after first.\n"));
    }
    if(const auto* xml_editorsettings = m_rootXml->FirstChildElement("editorsettings"); xml_editorsettings != nullptr) {
        DataUtils::ValidateXmlElement(*xml_editorsettings, "editorsettings", "", "", "chunksize,export", "");
        if(const auto chunksize_count = DataUtils::GetChildElementCount(*xml_editorsettings, "chunksize"); chunksize_count > 1) {
            DebuggerPrintf(std::format("WARNING: TMX map file editorsettings element contains more than one \"chunksize\" element. Ignoring all after the first.\n"));
        }
        if(const auto export_count = DataUtils::GetChildElementCount(*xml_editorsettings, "export"); export_count > 1) {
            DebuggerPrintf(std::format("WARNING: TMX map file editorsettings element contains more than one \"export\" child element. Ignoring all after the first.\n"));
        }
        if(const auto* xml_chunksize = xml_editorsettings->FirstChildElement("chunksize"); xml_chunksize != nullptr) {
            DataUtils::ValidateXmlElement(*xml_chunksize, "chunksize", "", "", "", "width,height");
            description.chunkWidth = static_cast<uint16_t>(DataUtils::ParseXmlAttribute(*xml_chunksize, "width", 16));
            description.chunkHeight = static_cast<uint16_t>(DataUtils::ParseXmlAttribute(*xml_chunksize, "height", 16));
        }
        if(const auto* xml_export = xml_editorsettings->FirstChildElement("export"); xml_export != nullptr) {
            DataUtils::ValidateXmlElement(*xml_export, "export", "", "target,format");
            description.exportTarget = DataUtils::GetAttributeAsString(*xml_export, "target");
            DebuggerPrintf(std::format("Map last exported as: {}.\n", description.exportTarget));
            description.exportFormat = DataUtils::GetAttributeAsString(*xml_export, "format");
            DebuggerPrintf(std::format("Map last formatted as: {}.\n", description.exportFormat));
        }

    }

    const auto* xml_tileset = m_rootXml->FirstChildElement("tileset");
    const auto&& [firstgid, filepath] = ParseTilesetElement(*xml_tileset);
    TsxReader tileReader{filepath};
    tileReader.description.firstGid = firstgid;
    tileReader.Parse();

    ParseLayerElements(map, *m_rootXml, tileReader.description);

    //if(DataUtils::HasChild(*m_rootXml, "layer")) {
    //    ParseTmxTileLayerElements(*m_rootXml, firstgid);
    //}
    //if(DataUtils::HasChild(*m_rootXml, "objectgroup")) {

    //}
    //if(DataUtils::HasChild(*m_rootXml, "imagelayer")) {

    //}
    //if(DataUtils::HasChild(*m_rootXml, "group")) {

    //}
}

std::pair<int, std::filesystem::path> TmxReader::ParseTilesetElement(const XMLElement& elem) noexcept {
    DataUtils::ValidateXmlElement(elem, "tileset", "", "firstgid,source", "", "");
    auto firstgid = DataUtils::ParseXmlAttribute(elem, "firstgid", 1);
    const auto src = [this, &elem]() {
        auto raw_src = std::filesystem::path{ DataUtils::ParseXmlAttribute(elem, "source", std::string{}) };
        if(!raw_src.has_parent_path() || (raw_src.parent_path() != m_filepath.parent_path())) {
            raw_src = std::filesystem::canonical(m_filepath.parent_path() / raw_src);
        }
        raw_src.make_preferred();
        return raw_src;
    }();
    return std::make_pair(firstgid, src);
}

void TmxReader::ParseLayerElements(Map& map, const XMLElement& elem, const TsxDesc& tsxDescription) noexcept {
    const auto map_width = DataUtils::ParseXmlAttribute(elem, "width", min_map_width);
    const auto map_height = DataUtils::ParseXmlAttribute(elem, "height", min_map_height);
    if(const auto count = DataUtils::GetChildElementCount(elem, "layer"); count > 9) {
        g_theFileLogger->LogWarnLine(std::format("Layer count of TMX map {0} is greater than the maximum allowed ({1}).\nOnly the first {1} layers will be used.", tsxDescription.name, Map::max_layers));
        g_theFileLogger->Flush();
    }
    DataUtils::ForEachChildElement(elem, "layer", [this, &map, map_width, map_height, tsxDescription](const XMLElement& xml_layer) {
        DataUtils::ValidateXmlElement(xml_layer, "layer", "", "width,height", "properties,data", "id,name,class,x,y,opacity,visible,locked,tintcolor,offsetx,offsety,parallaxx,parallaxy");
    if(DataUtils::HasAttribute(xml_layer, "x") || DataUtils::HasAttribute(xml_layer, "y")) {
        g_theFileLogger->LogWarnLine(std::string{ "Attributes \"x\" and \"y\" in the layer element are deprecated and unsupported. Remove both attributes to suppress this message." });
        g_theFileLogger->Flush();
    }
    const auto layer_name = DataUtils::ParseXmlAttribute(xml_layer, "name", std::string{});
    if(DataUtils::HasChild(xml_layer, "properties")) {
        if(DataUtils::GetChildElementCount(xml_layer, "properties") > 1) {
            g_theFileLogger->LogWarnLine(std::format("WARNING: TMX map file layer element \"{}\" contains more than one \"properties\" element. Ignoring all after the first.\n", layer_name));
            g_theFileLogger->Flush();
        }
    }
    if(DataUtils::HasChild(xml_layer, "data")) {
        if(DataUtils::GetChildElementCount(xml_layer, "data") > 1) {
            g_theFileLogger->LogWarnLine(std::format("WARNING: TMX map file layer element \"{}\" contains more than one \"data\" element. Ignoring all after the first.\n", layer_name));
            g_theFileLogger->Flush();
        }
    }

    const auto layer_width = DataUtils::ParseXmlAttribute(xml_layer, "width", map_width);
    const auto layer_height = DataUtils::ParseXmlAttribute(xml_layer, "height", map_height);
    
    map._layers.emplace_back(std::move(std::make_unique<Layer>(&map, IntVector2{ layer_width, layer_height })));
    auto* layer = map._layers.back().get();
    const auto clr_str = DataUtils::ParseXmlAttribute(xml_layer, "tintcolor", std::string{});
    layer->color.SetRGBAFromARGB(clr_str);
    layer->z_index = static_cast<int>(map._layers.size()) - std::size_t{ 1u };
    if(DataUtils::HasChild(xml_layer, "data")) {
        auto* xml_data = xml_layer.FirstChildElement("data");
        InitializeTilesFromTmxData(layer, *xml_data, tsxDescription.firstGid);
    }
    });
}

void TmxReader::InitializeTilesFromTmxData(Layer* layer, const XMLElement& elem, int firstgid) noexcept {
    DataUtils::ValidateXmlElement(elem, "data", "", "", "tile,chunk", "encoding,compression");
    const auto encoding = DataUtils::GetAttributeAsString(elem, "encoding");
    const auto compression = DataUtils::GetAttributeAsString(elem, "compression");
    const auto is_xml = encoding.empty();
    const auto is_csv = encoding == std::string{ "csv" };
    const auto is_base64 = encoding == "base64";
    const auto is_base64gzip = is_base64 && compression == "gzip";
    const auto is_base64zlib = is_base64 && compression == "zlib";
    const auto is_base64zstd = is_base64 && compression == "zstd";
    if(is_xml) {
        g_theFileLogger->LogWarnLine("TMX Map data as XML is deprecated.");
        g_theFileLogger->Flush();
        std::size_t tile_index{ 0u };
        DataUtils::ForEachChildElement(elem, "tile", [layer, &tile_index](const XMLElement& tile_elem) {
            if(DataUtils::HasAttribute(tile_elem, "gid")) {
                const auto tile_gid = DataUtils::ParseXmlAttribute(tile_elem, "gid", 0);
                if(auto* tile = layer->GetTile(tile_index); tile != nullptr) {
                    if(tile_gid > 0 && tile_gid != std::size_t(-1)) {
                        tile->ChangeTypeFromId(tile_gid);
                    } else {
                        tile->ChangeTypeFromName("void");
                    }
                } else {
                    ERROR_AND_DIE("Too many tiles.");
                }
            } else {
                if(auto* tile = layer->GetTile(tile_index); tile != nullptr) {
                    tile->ChangeTypeFromName("void");
                } else {
                    ERROR_AND_DIE("Too many tiles.");
                }
            }
        ++tile_index;
        });
    } else if(is_csv) {
        const auto data_text = StringUtils::RemoveAllWhitespace(DataUtils::GetElementTextAsString(elem));
        std::size_t tile_index{ 0u };
        for(const auto& gid : StringUtils::Split(data_text)) {
            if(auto* tile = layer->GetTile(tile_index); tile != nullptr) {
                const auto gidAsId = static_cast<std::size_t>(std::stoull(gid));
                if(gidAsId > 0) {
                    tile->ChangeTypeFromId(gidAsId - firstgid);
                }
            } else {
                ERROR_AND_DIE("Too many tiles.");
            }
            ++tile_index;
        }
    } else if(is_base64) {
        const auto encoded_data_text = StringUtils::RemoveAllWhitespace(DataUtils::GetElementTextAsString(elem));
        auto output = std::vector<uint8_t>{};
        FileUtils::Base64::Decode(encoded_data_text, output);
        const auto width = static_cast<std::size_t>(layer->tileDimensions.x);
        const auto height = static_cast<std::size_t>(layer->tileDimensions.y);
        const auto valid_data_size = width * height * std::size_t{ 4u };
        const auto err_msg = std::format("Invalid decoded Layer data: Size of data ({}) does not equal {} * {} * 4 or {}", output.size(), width, height, valid_data_size);
        GUARANTEE_OR_DIE(output.size() == valid_data_size, err_msg.c_str());

        std::size_t tile_index{ 0u };
        constexpr auto flag_flipped_horizontally = uint32_t{ 0x80000000u };
        constexpr auto flag_flipped_vertically = uint32_t{ 0x40000000u };
        constexpr auto flag_flipped_diagonally = uint32_t{ 0x20000000u };
        constexpr auto flag_rotated_hexagonal_120 = uint32_t{ 0x10000000u };
        constexpr auto flag_mask = flag_flipped_horizontally | flag_flipped_vertically | flag_flipped_diagonally | flag_rotated_hexagonal_120;
        for(std::size_t y = 0; y < height; ++y) {
            for(std::size_t x = 0; x < width; ++x) {
                auto gid = output[tile_index + 0] << 0 |
                    output[tile_index + 1] << 8 |
                    output[tile_index + 2] << 16 |
                    output[tile_index + 3] << 24;

                tile_index += 4;

                gid &= ~flag_mask;
                if(auto* tile = layer->GetTile(layer->GetTileIndex(x, y)); tile != nullptr) {
                    if(firstgid <= gid) {
                        tile->ChangeTypeFromId(static_cast<std::size_t>(gid) - firstgid);
                    }
                }
            }
        }
    } else if(is_base64gzip) {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    } else if(is_base64zlib) {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    } else if(is_base64zstd) {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    } else {
        ERROR_AND_DIE("Layer compression is not yet supported. Resave the .tmx file with with no compression.");
    }
}
