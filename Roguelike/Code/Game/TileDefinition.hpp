#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <map>
#include <memory>
#include <string>

namespace a2de {
    class AnimatedSprite;
    class Renderer;
    class SpriteSheet;
    class Texture;
}

class TileDefinition {
public:
    TileDefinition() = delete;
    TileDefinition(const TileDefinition& other) = default;
    TileDefinition(TileDefinition&& other) = default;
    TileDefinition& operator=(const TileDefinition& other) = default;
    TileDefinition& operator=(TileDefinition&& other) = default;
    ~TileDefinition() = default;

    static TileDefinition* CreateTileDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet);
    static void DestroyTileDefinitions();

    static TileDefinition* GetTileDefinitionByName(const std::string& name);
    static TileDefinition* GetTileDefinitionByGlyph(char glyph);
    static void ClearTileRegistry();

    bool is_opaque = false;
    bool is_visible = true;
    bool is_solid = false;
    bool is_animated = false;
    bool is_transparent = false;
    bool allow_diagonal_movement = true;
    char glyph = ' ';
    std::string name{};
    int frame_length = 0;

    const a2de::Texture* GetTexture() const;
    a2de::Texture* GetTexture();
    const a2de::SpriteSheet* GetSheet() const;
    a2de::SpriteSheet* GetSheet();
    const a2de::AnimatedSprite* GetSprite() const;
    a2de::AnimatedSprite* GetSprite();
    a2de::IntVector2 GetIndexCoords() const;
    int GetIndex() const;

    TileDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet);
protected:
private:
    bool LoadFromXml(const a2de::XMLElement& elem);
    void SetIndex(int index);
    void SetIndex(int x, int y);
    void SetIndex(const a2de::IntVector2& indexCoords);
    void AddOffsetToIndex(int offset);

    static std::map<std::string, std::unique_ptr<TileDefinition>> s_registry;
    a2de::Renderer& _renderer;
    std::weak_ptr<a2de::SpriteSheet> _sheet{};
    std::unique_ptr<a2de::AnimatedSprite> _sprite{};
    a2de::IntVector2 _index{};
    int _random_index_offset = 0;
    
};
