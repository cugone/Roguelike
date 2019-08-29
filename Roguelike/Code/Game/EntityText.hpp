#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/TimeUtils.hpp"

#include "Game/Entity.hpp"

#include <vector>
#include <memory>

class KerningFont;

struct TextEntityDesc {
    std::string text = "DAMAGE";
    Rgba color = Rgba::White;
    Vector2 position{ 0.0f, 0.0f };
    TimeUtils::FPSeconds timeToLive{ 1.0f };
    KerningFont* font{};
};

class EntityText : public Entity {
public:
    std::string text{};
    Vector2 worldPosition{};
    TimeUtils::FPSeconds ttl{1.0f};
    KerningFont* font{};
    virtual ~EntityText() = default;

    virtual void Update(TimeUtils::FPSeconds deltaSeconds) override;
    virtual void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const override;

protected:
private:
    TimeUtils::FPSeconds _currentLiveTime{};
};
