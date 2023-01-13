#include "Game/TsxReader.hpp"

#include "Engine/Core/EngineCommon.hpp"

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/FileLogger.hpp"

#include "Game/Game.hpp"
#include "Game/GameCommon.hpp"
#include "Game/TileDefinition.hpp"

TsxReader::TsxReader(std::filesystem::path filepath) noexcept {
    description.filepath = filepath;
    LoadFile();
}

bool TsxReader::LoadFile() noexcept {
    return LoadFile(description.filepath);
}

bool TsxReader::LoadFile(std::filesystem::path filepath) noexcept {
    if(!FileUtils::IsSafeReadPath(filepath)) {
        DebuggerPrintf(std::format("WARNING: TSX tileset file \"{:s}\" could not be parsed.\n", description.filepath, m_xmlDoc.ErrorStr()));
        return false;
    }
    filepath = std::filesystem::canonical(filepath);
    description.filepath = filepath;
    m_rootElement = nullptr;
    if(tinyxml2::XML_SUCCESS != m_xmlDoc.LoadFile(description.filepath.string().c_str())) {
        DebuggerPrintf(std::format("WARNING: TSX tileset file \"{:s}\" could not be parsed. XML parser returned: {:s}\n", description.filepath, m_xmlDoc.ErrorStr()));
        return false;
    }
    m_rootElement = m_xmlDoc.RootElement();
    return true;
}

void TsxReader::LoadTmxTileset(const XMLElement& elem) noexcept {
    PROFILE_BENCHMARK_FUNCTION();
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

        constexpr auto required_tsx_version = "1.9";
        verify_version(elem, "version", required_tsx_version);

        constexpr auto required_tiled_version = "1.9.2";
        verify_version(elem, "tiledversion", required_tiled_version);
    }

    TileDefinitionDesc desc{};
    const auto tilecount = DataUtils::ParseXmlAttribute(elem, "tilecount", 1);
    const auto columncount = DataUtils::ParseXmlAttribute(elem, "columns", 1);
    const auto width = columncount;
    const auto height = tilecount / columncount;
    if(const auto* xml_image = elem.FirstChildElement("image"); xml_image != nullptr) {
        //Attribute "id" is deprecated and unsupported. It is an error if it exists.
        DataUtils::ValidateXmlElement(*xml_image, "image", "", "source,width,height", "data", "id,format,trans");
        if(DataUtils::HasAttribute(elem, "id")) {
            g_theFileLogger->LogWarnLine(std::string{ "Attribute \"id\" in the image element is deprecated and unsupported. Remove the attribute to suppress this message." });
        }
        auto src = std::filesystem::path{ DataUtils::ParseXmlAttribute(*xml_image, "source", std::string{}) };
        if(!src.has_parent_path() || (src.parent_path() != description.filepath.parent_path())) {
            src = std::filesystem::canonical(description.filepath.parent_path() / src);
        }
        src.make_preferred();
        GetGameAs<Game>()->_tileset_sheet = g_theRenderer->CreateSpriteSheet(src, width, height);
        //TileDefinition::CreateTileDefinition();

    }
    DataUtils::ForEachChildElement(elem, "tile", [&desc, width](const XMLElement& xml_tile) {
        const auto tile_idx = DataUtils::ParseXmlAttribute(xml_tile, "id", 0);
    desc.tileId = std::size_t(tile_idx);
    if(DataUtils::HasChild(xml_tile, "animation")) {
        desc.animated = true;
        if(const auto* xml_animation = xml_tile.FirstChildElement("animation"); xml_animation != nullptr) {
            TimeUtils::FPSeconds duration{ 0.0f };
            auto start_idx = 0;
            const auto length = static_cast<int>((std::max)(std::size_t{ 0u }, DataUtils::GetChildElementCount(*xml_animation, "frame")));
            desc.frame_length = length;
            if(DataUtils::HasChild(*xml_animation, "frame")) {
                const auto xml_frame = xml_animation->FirstChildElement("frame");
                start_idx = DataUtils::ParseXmlAttribute(*xml_frame, "tileid", 0);
                GUARANTEE_OR_DIE(start_idx == tile_idx, "First animation tile index must match selected tile index.");
                desc.anim_start_idx = start_idx;
                DataUtils::ForEachChildElement(*xml_animation, "frame", [&duration](const XMLElement& frame_elem) {
                    duration += TimeUtils::FPMilliseconds{ DataUtils::ParseXmlAttribute(frame_elem, "duration", 0) };
                });
                desc.anim_duration = duration.count();
            }
        }
    }
    if(DataUtils::HasChild(xml_tile, "properties")) {
        if(const auto* xml_properties = xml_tile.FirstChildElement("properties"); xml_properties != nullptr) {
            DataUtils::ForEachChildElement(*xml_properties, "property", [&](const XMLElement& property_elem) {
                DataUtils::ValidateXmlElement(property_elem, "property", "", "name,value", "properties", "propertytype,type");
            if(DataUtils::HasAttribute(property_elem, "type")) {
                DataUtils::ValidateXmlAttribute(property_elem, "type", "bool,color,class,float,file,int,object,string");
                if(const auto type_str = DataUtils::ParseXmlAttribute(property_elem, "type", std::string{ "string" }); !type_str.empty()) {
                    if(type_str == "bool") {
                        DataUtils::ValidateXmlAttribute(property_elem, "value", "true,false");
                        const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                        const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", false);
                        if(name == "allowDiagonalMovement") {
                            desc.allow_diagonal_movement = value;
                        } else if(name == "opaque") {
                            desc.opaque = value;
                        } else if(name == "solid") {
                            desc.solid = value;
                        } else if(name == "visible") {
                            desc.visible = value;
                        } else if(name == "transparent") {
                            desc.transparent = value;
                        } else if(name == "invisible") {
                            desc.visible = !value;
                        } else if(name == "entrance") {
                            desc.is_entrance = value;
                        } else if(name == "exit") {
                            desc.is_exit = value;
                        }
                    } else if(type_str == "int") {
                        const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                        const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", 0);
                        if(name == "light") {
                            desc.light = value;
                        } else if(name == "selflight") {
                            desc.self_illumination = value;
                        }
                    } else if(type_str == "color") {
                        //const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                        //const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", std::string{"#FFFFFFFF"});
                        //if(name == "tint") {
                        //    auto t = Rgba::NoAlpha;
                        //    t.SetRGBAFromARGB(value.empty() ? "#FFFFFFFF" : value);
                        //}
                    } else if(type_str == "file") {

                    } else if(type_str == "object") {

                    } else if(type_str == "class") {
                    }
                }
            } else { //No type attribute is a string
                const auto name = DataUtils::ParseXmlAttribute(property_elem, "name", std::string{});
                const auto value = DataUtils::ParseXmlAttribute(property_elem, "value", std::string{});
                if(name == "name") {
                    desc.name = value;
                } else if(name == "animName") {
                    desc.animName = value;
                } else if(name == "glyph") {
                    desc.glyph = value.empty() ? ' ' : value.front();
                }
            }
            });
        }
    }
    constexpr auto fmt = R"(<tileDefinition name="{:s}" index="[{},{}]"><glyph value="{}" /><animation name="{:s}"><animationset startindex="{}" framelength="{}" duration="{}" loop="true" /></animation></tileDefinition>)";
    const auto anim_str = std::vformat(fmt, std::make_format_args(desc.name, desc.tileId % width, desc.tileId / width, desc.glyph, desc.animName, desc.anim_start_idx, desc.frame_length, desc.anim_duration));
    tinyxml2::XMLDocument d;
    d.Parse(anim_str.c_str(), anim_str.size());
    if(const auto* xml_root = d.RootElement(); xml_root != nullptr) {
        if(auto* def = TileDefinition::CreateOrGetTileDefinition(*xml_root, GetGameAs<Game>()->_tileset_sheet); def && def->GetSprite() && !def->GetSprite()->GetMaterial()) {
            def->GetSprite()->SetMaterial(GetGameAs<Game>()->GetDefaultTileMaterial());
        }
    }
    });
}

void TsxReader::Parse() noexcept {
    PROFILE_BENCHMARK_FUNCTION();
    if(m_rootElement == nullptr) {
        DebuggerPrintf(std::format("WARNING: TSX tileset file failed to parse. No file loaded.\n"));
        return;
    }
    if(const auto* xml_tileset = m_rootElement; xml_tileset != nullptr) {
        DataUtils::ValidateXmlElement(*xml_tileset, "tileset", "", "name,tilewidth,tileheight,tilecount,columns", "image,tileoffset,grid,properties,terraintypes,wangsets,transformations", "class,spacing,margin,objectalignment,tilerendersize,fillmode");
        LoadTmxTileset(*xml_tileset);
    }
}
