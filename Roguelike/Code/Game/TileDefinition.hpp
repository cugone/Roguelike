#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <map>
#include <memory>
#include <string>

class SpriteSheet;
class Texture;

class TileDefinition {
public:
    TileDefinition() = delete;
    TileDefinition(const TileDefinition& other) = default;
    TileDefinition(TileDefinition&& other) = default;
    TileDefinition& operator=(const TileDefinition& other) = default;
    TileDefinition& operator=(TileDefinition&& other) = default;
    ~TileDefinition() = default;

    static void CreateTileDefinition(const XMLElement& elem, SpriteSheet* sheet);
    static void DestroyTileDefinitions();

    static TileDefinition* GetTileDefinitionByName(const std::string& name);
    static TileDefinition* GetTileDefinitionByGlyph(char glyph);
    static void ClearTileRegistry();

    bool is_opaque = false;
    bool is_visible = true;
    bool is_solid = false;
    bool allow_diagonal_movement = true;
    char glyph = ' ';
    std::string name{};
    IntVector2 index{};

    const Texture* GetTexture() const;
    Texture* GetTexture();
    const SpriteSheet* GetSheet() const;
    SpriteSheet* GetSheet();

    TileDefinition(const XMLElement& elem, SpriteSheet* sheet);
protected:
private:
    bool LoadFromXml(const XMLElement& elem);

    SpriteSheet* _sheet = nullptr;
    static std::map<std::string, std::unique_ptr<TileDefinition>> s_registry;
};
