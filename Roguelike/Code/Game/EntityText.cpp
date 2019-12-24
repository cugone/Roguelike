#include "Game/EntityText.hpp"

#include "Engine/Core/KerningFont.hpp"

#include "Game/GameCommon.hpp"

EntityText::EntityText(const TextEntityDesc& desc) noexcept
    : Entity()
    , text{desc.text}
    , screenPosition{desc.position}
    , ttl{desc.timeToLive}
    , font{desc.font}
    , speed{desc.speed}
{

}

void EntityText::Update(TimeUtils::FPSeconds deltaSeconds) {
    screenPosition += Vector2{0.0f, speed} * deltaSeconds.count();
    color.a = static_cast<unsigned char>(255.0f * std::clamp(1.0f - (_currentLiveTime.count() / ttl.count()), 0.0f, 1.0f));
    _currentLiveTime += deltaSeconds;
    if(ttl < _currentLiveTime) {
        this->Entity::GetBaseStats().MultiplyStat(StatsID::Health, 0.0L);
    }
}

void EntityText::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t /*layer_index*/) const {
    g_theRenderer->AppendMultiLineTextBuffer(font, text, screenPosition, layer_color, verts, ibo);
    const auto S = Matrix4::CreateScaleMatrix(1.0f);
    const auto R = Matrix4::I;
    const auto T = Matrix4::CreateTranslationMatrix(screenPosition);
    const auto M = Matrix4::MakeSRT(S, R, T);
    g_theRenderer->SetModelMatrix(M);
    g_theRenderer->DrawMultilineText(font, text, color);
}

EntityText* EntityText::CreateTextEntity(const TextEntityDesc& desc) {
    auto* ptr = new EntityText(desc);
    s_registry.emplace_back(ptr);
    return ptr;
}

void EntityText::ClearTextRegistry() noexcept {
    s_registry.clear();
}

