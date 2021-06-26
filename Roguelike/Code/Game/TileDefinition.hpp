#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <map>
#include <memory>
#include <string>


class AnimatedSprite;
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

    static TileDefinition* CreateOrGetTileDefinition(const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet);
    static TileDefinition* CreateTileDefinition(const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet);
    static void ClearTileDefinitions();

    static TileDefinition* GetTileDefinitionByName(const std::string& name);
    static TileDefinition* GetTileDefinitionByGlyph(char glyph);

    bool is_opaque = false;
    bool is_visible = true;
    bool is_solid = false;
    bool is_animated = false;
    bool is_transparent = false;
    bool is_entrance = false;
    bool is_exit = false;
    bool allow_diagonal_movement = true;
    char glyph = ' ';
    uint32_t light{};
    uint32_t self_illumination{};
    std::string name{};
    int frame_length = 0;

    uint32_t GetLightingBits() const noexcept;
    const Texture* GetTexture() const;
    Texture* GetTexture();
    const SpriteSheet* GetSheet() const;
    SpriteSheet* GetSheet();
    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();
    IntVector2 GetIndexCoords() const;
    int GetIndex() const;

    TileDefinition(const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet);
protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    void SetIndex(int index);
    void SetIndex(int x, int y);
    void SetIndex(const IntVector2& indexCoords);
    void AddOffsetToIndex(int offset);

    static std::map<std::string, std::unique_ptr<TileDefinition>> s_registry;
    std::weak_ptr<SpriteSheet> _sheet{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    IntVector2 _index{};
    int _random_index_offset = 0;

};
