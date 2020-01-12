#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <map>
#include <memory>
#include <string>

class AnimatedSprite;
class Renderer;
class SpriteSheet;
class Texture;

class CursorDefinition {
public:
    CursorDefinition() = delete;
    CursorDefinition(const CursorDefinition& other) = default;
    CursorDefinition(CursorDefinition&& other) = default;
    CursorDefinition& operator=(const CursorDefinition& other) = default;
    CursorDefinition& operator=(CursorDefinition&& other) = default;
    ~CursorDefinition() = default;

    static const std::map<std::string, std::unique_ptr<CursorDefinition>>& GetLoadedDefinitions();
    static void CreateCursorDefinition(Renderer& renderer, const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet);
    static void DestroyCursorDefinitions();

    static CursorDefinition* GetCursorDefinitionByName(const std::string& name);
    static void ClearCursorRegistry();

    std::string name{};
    int frame_length = 0;
    bool is_animated = false;

    const Texture* GetTexture() const;
    Texture* GetTexture();
    const SpriteSheet* GetSheet() const;
    SpriteSheet* GetSheet();
    const AnimatedSprite* GetSprite() const;
    AnimatedSprite* GetSprite();
    IntVector2 GetIndexCoords() const;
    int GetIndex() const;

    CursorDefinition(Renderer& renderer, const XMLElement& elem, std::weak_ptr<SpriteSheet> sheet);

protected:
private:
    bool LoadFromXml(const XMLElement& elem);
    void SetIndex(int index);
    void SetIndex(int x, int y);
    void SetIndex(const IntVector2& indexCoords);

    static std::map<std::string, std::unique_ptr<CursorDefinition>> s_registry;
    Renderer& _renderer;
    std::weak_ptr<SpriteSheet> _sheet{};
    std::unique_ptr<AnimatedSprite> _sprite{};
    IntVector2 _index{};

};
