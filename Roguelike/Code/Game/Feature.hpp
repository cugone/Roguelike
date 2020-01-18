#pragma once

#include "Game/Entity.hpp"

#include <map>

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

    virtual void Update(TimeUtils::FPSeconds deltaSeconds) override;
    virtual void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const override;

    bool ToggleSolid() noexcept;
    bool IsSolid() const noexcept;

    bool ToggleOpaque() noexcept;
    bool IsOpaque() const noexcept;

    bool ToggleVisible() noexcept;
    bool IsVisible() const;
    bool IsNotVisible() const;
    bool IsInvisible() const;

    virtual void SetPosition(const IntVector2& position) override;

protected:
    virtual void ResolveAttack(Entity& attacker, Entity& defender) override;

private:
    bool LoadFromXml(const XMLElement& elem);

    TileDefinition* _tile_def{};
    bool _isSolid{false};
    bool _isOpaque{false};
    bool _isVisible{true};
    static std::map<std::string, std::unique_ptr<Feature>> s_registry;
};
