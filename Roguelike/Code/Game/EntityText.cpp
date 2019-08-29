#include "Game/EntityText.hpp"

#include "Engine/Core/KerningFont.hpp"

#include "Game/GameCommon.hpp"

void EntityText::Update(TimeUtils::FPSeconds deltaSeconds) {
    worldPosition += Vector2{0.0f, 1.0f} * deltaSeconds.count();
    color.a = static_cast<unsigned char>(255.0f * std::clamp(1.0f - (_currentLiveTime.count() / ttl.count()), 0.0f, 1.0f));
    _currentLiveTime += deltaSeconds;
    if(ttl < _currentLiveTime) {
        
    }
}

void EntityText::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t /*layer_index*/) const {
    g_theRenderer->AppendMultiLineTextBuffer(font, text, worldPosition, layer_color, verts, ibo);
}

