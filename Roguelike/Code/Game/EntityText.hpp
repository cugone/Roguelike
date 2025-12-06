#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Game/Entity.hpp"

#include <vector>
#include <memory>

namespace a2de {
    class IFont;
}

struct TextEntityDesc {
    std::string text = "DAMAGE";
    Rgba color = Rgba::White;
    Vector2 position{0.0f, 0.0f};
    TimeUtils::FPSeconds timeToLive{1.0f};
    a2de::IFont* font{nullptr};
    float speed{1.0f};
};

class EntityText : public Entity {
public:
    std::string text{};
    TimeUtils::FPSeconds ttl{1.0f};
    Rgba color{Rgba::White};
    a2de::IFont* font{};
    float speed{1.0f};


    static EntityText* CreateTextEntity(const TextEntityDesc& desc);
    static void ClearTextRegistry() noexcept;

    EntityText() = default;
    explicit EntityText(const TextEntityDesc& desc) noexcept;
    virtual ~EntityText() = default;

    void Update(TimeUtils::FPSeconds deltaSeconds) override;
    void Render() const;
    void EndFrame() override;

protected:
private:

    static inline std::vector<std::unique_ptr<EntityText>> s_registry{};
    TimeUtils::FPSeconds _currentLiveTime{};
};
