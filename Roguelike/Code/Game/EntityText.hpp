#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Game/Entity.hpp"

#include <vector>
#include <memory>

namespace a2de {
    class KerningFont;
}

struct TextEntityDesc {
    std::string text = "DAMAGE";
    a2de::Rgba color = a2de::Rgba::White;
    a2de::Vector2 position{ 0.0f, 0.0f };
    a2de::TimeUtils::FPSeconds timeToLive{ 1.0f };
    a2de::KerningFont* font{nullptr};
    float speed{1.0f};
};

class EntityText : public Entity {
public:
    std::string text{};
    a2de::TimeUtils::FPSeconds ttl{1.0f};
    a2de::Rgba color{a2de::Rgba::White};
    a2de::KerningFont* font{};
    float speed{1.0f};
    

    static EntityText* CreateTextEntity(const TextEntityDesc& desc);
    static void ClearTextRegistry() noexcept;

    EntityText() = default;
    explicit EntityText(const TextEntityDesc & desc) noexcept;
    virtual ~EntityText() = default;

    void Update(a2de::TimeUtils::FPSeconds deltaSeconds) override;
    void Render() const;
    void EndFrame() override;

protected:
private:

    static inline std::vector<std::unique_ptr<EntityText>> s_registry{};
    a2de::TimeUtils::FPSeconds _currentLiveTime{};
};
