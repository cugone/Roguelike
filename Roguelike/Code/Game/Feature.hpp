#pragma once

#include "Game/Entity.hpp"

#include <map>
#include <vector>

class TileDefinition;

class Feature : public Entity {
public:
    Feature() = delete;
    Feature(const Feature& other) = default;
    Feature(Feature&& other) = default;
    Feature& operator=(const Feature& other) = default;
    Feature& operator=(Feature&& other) = default;
    virtual ~Feature() = default;

    static Feature* CreateFeature(Map* map, const XMLElement& elem);
    static void ClearFeatureRegistry();

    static Feature* GetFeatureByName(const std::string& name);
    static Feature* GetFeatureByGlyph(const char glyph);

    Feature(Map* map, const XMLElement& elem) noexcept;

    bool IsSolid() const noexcept;
    bool IsOpaque() const noexcept;
    bool IsVisible() const noexcept;
    bool IsInvisible() const noexcept;

    virtual void SetPosition(const IntVector2& position) override;
    void SetState(const std::string& stateName);

    void CalculateLightValue() noexcept override;
    void AddVertsForSelf() noexcept override;
    void AddVerts() noexcept override;

    Tile* parent_tile{};

protected:
    virtual void ResolveAttack(Entity& attacker, Entity& defender) override;

private:
    bool LoadFromXml(const XMLElement& elem);

    std::vector<std::string> _states{};
    decltype(_states)::iterator _current_state{};
    static std::map<std::string, std::unique_ptr<Feature>> s_registry;
};
