#include "Game/EntityText.hpp"

#include "Engine/Core/KerningFont.hpp"

#include "Game/GameCommon.hpp"

#include <algorithm>

EntityText::EntityText(const TextEntityDesc& desc) noexcept
    : Entity()
    , text{desc.text}
    , ttl{desc.timeToLive}
    , color{desc.color}
    , font{desc.font}
    , speed{desc.speed}
{
    _position = IntVector2{desc.position};
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

void EntityText::Render() const {
    const auto S = Matrix4::I;
    const auto R = Matrix4::I;
    const auto worldCoords = _screen_position;
    const auto screen_position = g_theRenderer->ConvertWorldToScreenCoords(map->cameraController.GetCamera(), worldCoords);
    const auto text_width = font->CalculateTextWidth(text);
    const auto text_height = font->CalculateTextHeight(text);
    const auto text_half_extents = Vector2{text_width, text_height} * 0.5f;
    const auto text_center = screen_position - text_half_extents;
    const auto T = Matrix4::CreateTranslationMatrix(text_center);
    const auto M = Matrix4::MakeSRT(S, R, T);
    g_theRenderer->SetModelMatrix(M);
    g_theRenderer->DrawTextLine(font, text, color);
}

void EntityText::EndFrame() {
    for(auto* e : this->map->GetTextEntities()) {
        if(e != this) {
            continue;
        }
        if(e->GetStats().GetStat(StatsID::Health) <= 0) {
            e = nullptr;
        }
    }
    s_registry.erase(std::remove_if(std::begin(s_registry), std::end(s_registry), [this](const auto& t)->bool { return (t.get() == this) && t->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(s_registry));
}

EntityText* EntityText::CreateTextEntity(const TextEntityDesc& desc) {
    auto newEntityText = std::make_unique<EntityText>(desc);
    auto* ptr = newEntityText.get();
    s_registry.emplace_back(std::move(newEntityText));
    return ptr;
}

void EntityText::ClearTextRegistry() noexcept {
    s_registry.clear();
}

