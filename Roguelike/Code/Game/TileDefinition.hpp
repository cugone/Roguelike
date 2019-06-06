#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <map>
#include <memory>
#include <string>

class SpriteSheet;
class AnimatedSprite;
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
    bool is_animated = false;
    bool allow_diagonal_movement = true;
    char glyph = ' ';
    std::string name{};
    int frame_length = 0;

    const Texture* GetTexture() const;
    Texture* GetTexture();
    const SpriteSheet* GetSheet() const;
    SpriteSheet* GetSheet();
    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();
    IntVector2 GetIndexCoords() const;
    int GetIndex() const;

    TileDefinition(const XMLElement& elem, SpriteSheet* sheet);
protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    void SetIndex(int index);
    void SetIndex(int x, int y);
    void SetIndex(const IntVector2& indexCoords);
    void AddOffsetToIndex(int offset);
    IntVector2 _index{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    SpriteSheet* _sheet = nullptr;
    int _random_index_offset = 0;
    static std::map<std::string, std::unique_ptr<TileDefinition>> s_registry;
};
