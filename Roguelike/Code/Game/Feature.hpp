#pragma once

#include "Game/Entity.hpp"

class Feature : public Entity {
public:
    Feature() = delete;
    Feature(const Feature& other) = default;
    Feature(Feature&& other) = default;
    Feature& operator=(const Feature& other) = default;
    Feature& operator=(Feature&& other) = default;
    virtual ~Feature() = default;

    Feature(const XMLElement& elem) noexcept;

    virtual void Update(TimeUtils::FPSeconds deltaSeconds) override;
    virtual void Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t layer_index) const override;

    bool ToggleSolid() noexcept;
    bool IsSolid() const noexcept;

    bool ToggleOpaque() noexcept;
    bool IsOpaque() const noexcept;

    virtual void SetPosition(const IntVector2& position) override;

protected:
private:
    bool _isSolid{false};
    bool _isOpaque{false};
    
};
