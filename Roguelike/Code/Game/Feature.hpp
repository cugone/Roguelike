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

    Feature(Map* map, const XMLElement& elem) noexcept;

    bool IsTransparent() const noexcept;
    bool IsSolid() const noexcept;
    bool IsOpaque() const noexcept;
    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    virtual void SetPosition(const IntVector2& position) override;
    void SetState(const std::string& stateName);

protected:
    virtual void ResolveAttack(Entity& attacker, Entity& defender) override;

private:
    bool LoadFromXml(const XMLElement& elem);

    TileDefinition* _tile_def{};
    std::vector<std::string> _states{};
    static std::map<std::string, std::unique_ptr<Feature>> s_registry;
};
