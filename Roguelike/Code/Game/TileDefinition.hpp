#pragma once

#include "Engine/Core/DataUtils.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include <map>
#include <memory>
#include <string>


class AnimatedSprite;
class SpriteSheet;
class Texture;

struct TileDefinitionDesc {
    std::size_t tileId{4224u};
    std::string name{"void"};
    std::string animName{};
    bool opaque{false};
    bool visible{ true };
    bool solid{ false };
    bool animated{ false };
    bool transparent{ false };
    bool is_entrance{ false };
    bool is_exit{ false };
    bool allow_diagonal_movement{ false };
    char glyph{' '};
    uint32_t light{0u};
    uint32_t self_illumination{0u};
    int anim_start_idx{0};
    int frame_length{0};
    float anim_duration{TimeUtils::FPSeconds{TimeUtils::FPMilliseconds{16.0f}}.count()};
};

class TileDefinition {
public:
    TileDefinition() = delete;
    TileDefinition(const TileDefinition& other) = default;
    TileDefinition(TileDefinition&& other) = default;
    TileDefinition& operator=(const TileDefinition& other) = default;
    TileDefinition& operator=(TileDefinition&& other) = default;
    ~TileDefinition() = default;

    static TileDefinition* CreateOrGetTileDefinition(const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet);
    static TileDefinition* CreateTileDefinition(const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet);
    static void ClearTileDefinitions();

    static TileDefinition* GetTileDefinitionByName(const std::string& name);
    static TileDefinition* GetTileDefinitionByGlyph(char glyph);
    static TileDefinition* GetTileDefinitionByIndex(std::size_t index);

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
    std::size_t GetIndex() const;

    TileDefinition(const XMLElement& elem, std::shared_ptr<SpriteSheet> sheet);
protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    void SetIndex(std::size_t index);
    void SetIndex(int x, int y);
    void SetIndex(const IntVector2& indexCoords);
    void AddOffsetToIndex(std::size_t offset);

    static inline std::map<std::string, std::unique_ptr<TileDefinition>> s_registry{};
    std::shared_ptr<SpriteSheet> _sheet{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    IntVector2 _index{};
    std::size_t _random_index_offset = 0u;

};
