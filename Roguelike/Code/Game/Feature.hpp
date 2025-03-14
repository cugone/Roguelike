#pragma once

#include "Game/Entity.hpp"

#include <map>
#include <vector>

class TileDefinition;
class Feature;
class Map;
class Layer;
class Tile;

class FeatureInstance {
public:
    Tile* GetParentTile() const noexcept;

    void AddState() noexcept;
    void SetState(std::vector<std::string>::iterator iterator) noexcept;
    void SetStatebyName(const std::string& name) noexcept;

    const Feature* feature{};
    std::size_t layer_index{};
    std::size_t index{};
private:
    std::vector<std::string> states{};
    decltype(states)::iterator current_state{};

    friend class Feature;
};

class Feature : public Entity {
public:
    Feature() = delete;
    Feature(const Feature& other) = default;
    Feature(Feature&& other) = default;
    Feature& operator=(const Feature& other) = default;
    Feature& operator=(Feature&& other) = default;
    virtual ~Feature() = default;

    static Feature* CreateFeature(Map* map, const XMLElement& elem);
    static FeatureInstance CreateInstanceFromFeature(const Feature* feature) noexcept;
    static FeatureInstance CreateInstanceFromFeatureAt(const Feature* feature, const IntVector2& position) noexcept;
    static std::optional<FeatureInstance> CreateInstanceFromFeatureByName(std::string name) noexcept;
    static std::optional<FeatureInstance> CreateInstanceFromFeatureByNameAt(std::string name, const IntVector2& position) noexcept;
    static void ClearFeatureRegistry();

    static Feature* GetFeatureByName(const std::string& name);
    static Feature* GetFeatureByGlyph(const char glyph);

    std::string GetFullyQualifiedNameFromCurrentState() const noexcept;
    std::string GetFullyQualifiedNameFromState(std::string stateName) const noexcept;


    Feature(Map* map, const XMLElement& elem) noexcept;

    bool IsSolid() const noexcept override;
    bool IsOpaque() const noexcept override;
    bool IsVisible() const noexcept override;
    bool IsInvisible() const noexcept override;

    virtual void SetPosition(const IntVector2& position) override;
    void SetState(const std::string& stateName);

    void CalculateLightValue() noexcept override;

    FeatureInstance CreateInstance() const noexcept;
    FeatureInstance CreateInstanceAt(const IntVector2& position) const noexcept;

protected:
    void ResolveAttack(Entity& attacker, Entity& defender) override;
    void ApplyDamage(DamageType type, long amount, bool crit) override;
    void AttackerMissed() override;
    void OnDestroyed() override;

private:
    bool LoadFromXml(const XMLElement& elem);

    std::vector<std::string> _states{};
    decltype(_states)::iterator _current_state{};
    static inline std::multimap<std::string, std::unique_ptr<Feature>> s_registry = std::multimap<std::string, std::unique_ptr<Feature>>{};

    friend class FeatureInfo;
};

class FeatureInfo {
public:
    Layer* layer{nullptr};
    std::size_t index{0u};

    bool HasState(std::string stateName) const noexcept;
    bool HasStates() const noexcept;
    std::vector<std::string> GetStates() const noexcept;
    bool SetState(std::string newState) noexcept;
    std::string GetCurrentState() const noexcept;

protected:
private:
};
