#include "Game/EntityText.hpp"

#include "Engine/Core/KerningFont.hpp"

#include "Game/GameCommon.hpp"

#include <algorithm>

EntityText::EntityText(const TextEntityDesc& desc) noexcept
    : Entity()
    , text{desc.text}
    , ttl{desc.timeToLive}
    , font{desc.font}
    , speed{desc.speed}
{
    _screen_position = desc.position;
}

void EntityText::Update(TimeUtils::FPSeconds deltaSeconds) {
    _screen_position += Vector2{0.0f, -speed} * deltaSeconds.count();
    color.a = static_cast<unsigned char>(255.0f * std::clamp(1.0f - (_currentLiveTime.count() / ttl.count()), 0.0f, 1.0f));
    _currentLiveTime += deltaSeconds;
    if(ttl < _currentLiveTime) {
        this->Entity::GetBaseStats().MultiplyStat(StatsID::Health, 0.0L);
    }
}

void EntityText::Render(std::vector<Vertex3D>& verts, std::vector<unsigned int>& ibo, const Rgba& layer_color, size_t /*layer_index*/) const {
    g_theRenderer->AppendMultiLineTextBuffer(font, text, _screen_position, layer_color, verts, ibo);
    const auto S = Matrix4::I;
    const auto R = Matrix4::I;
    const auto T = Matrix4::CreateTranslationMatrix(_screen_position);
    const auto M = Matrix4::MakeSRT(S, R, T);
    g_theRenderer->SetModelMatrix(M);
    g_theRenderer->DrawMultilineText(font, text, color);
}

void EntityText::EndFrame() {
    s_registry.erase(std::remove_if(std::begin(s_registry), std::end(s_registry), [](const auto& t)->bool { return !t || (t && t->GetStats().GetStat(StatsID::Health) <= 0); }), std::end(s_registry));
}

EntityText* EntityText::CreateTextEntity(const TextEntityDesc& desc) {
    auto* ptr = new EntityText(desc);
    s_registry.emplace_back(ptr);
    return ptr;
}

void EntityText::ClearTextRegistry() noexcept {
    s_registry.clear();
}

