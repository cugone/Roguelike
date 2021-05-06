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

class CursorDefinition {
public:
    CursorDefinition() = delete;
    CursorDefinition(const CursorDefinition& other) = default;
    CursorDefinition(CursorDefinition&& other) = default;
    CursorDefinition& operator=(const CursorDefinition& other) = default;
    CursorDefinition& operator=(CursorDefinition&& other) = default;
    ~CursorDefinition() = default;

    static const std::vector<std::unique_ptr<CursorDefinition>>& GetLoadedDefinitions();
    static void CreateCursorDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet);
    static void DestroyCursorDefinitions();

    static CursorDefinition* GetCursorDefinitionByName(const std::string& name);
    static void ClearCursorRegistry();

    std::string name{};
    int frame_length = 0;
    bool is_animated = false;

    const a2de::Texture* GetTexture() const;
    a2de::Texture* GetTexture();
    const a2de::SpriteSheet* GetSheet() const;
    a2de::SpriteSheet* GetSheet();
    const a2de::AnimatedSprite* GetSprite() const;
    a2de::AnimatedSprite* GetSprite();
    a2de::IntVector2 GetIndexCoords() const;
    int GetIndex() const;

    CursorDefinition(a2de::Renderer& renderer, const a2de::XMLElement& elem, std::weak_ptr<a2de::SpriteSheet> sheet);

protected:
private:
    bool LoadFromXml(const a2de::XMLElement& elem);
    void SetIndex(int index);
    void SetIndex(int x, int y);
    void SetIndex(const a2de::IntVector2& indexCoords);

    static std::vector<std::unique_ptr<CursorDefinition>> s_registry;
    a2de::Renderer& _renderer;
    std::weak_ptr<a2de::SpriteSheet> _sheet{};
    std::unique_ptr<a2de::AnimatedSprite> _sprite{};
    a2de::IntVector2 _index{};

};
