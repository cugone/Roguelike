#include "Game/EntityText.hpp"

#include "Engine/Core/KerningFont.hpp"

#include "Game/GameCommon.hpp"
#include "Game/Actor.hpp"

#include <algorithm>

EntityText::EntityText(const TextEntityDesc& desc) noexcept
    : Entity()
    , text{desc.text}
    , ttl{desc.timeToLive}
    , color{desc.color}
    , font{desc.font}
    , speed{desc.speed}
{
    map = desc.map;
    _screen_position = desc.position;
}

void EntityText::Update(TimeUtils::FPSeconds deltaSeconds) {
    _screen_position += Vector2{0.0f, -speed} * deltaSeconds.count();
    color.a = static_cast<char>(255.0f * MathUtils::Interpolate(1.0f, 0.0f, _currentLiveTime.count() / ttl.count()));
    _currentLiveTime += deltaSeconds;
    if(ttl < _currentLiveTime) {
        this->Entity::GetBaseStats().MultiplyStat(StatsID::Health, 0.0L);
    }
}

void EntityText::Render() const {
    //TODO: Fix screen coord calc
    const auto S = Matrix4::I;
    const auto R = Matrix4::I;
    const auto pos = g_theRenderer->ConvertWorldToScreenCoords(Vector2{map->player->GetPosition()} + Vector2::ONE * 0.5f);
    const auto T = Matrix4::CreateTranslationMatrix(pos);
    const auto M = Matrix4::MakeSRT(S, R, T);
    g_theRenderer->SetModelMatrix(M);
    g_theRenderer->DrawTextLine(font, text, color);
}

void EntityText::EndFrame() {
    s_registry.erase(std::remove_if(std::begin(s_registry), std::end(s_registry), [](const auto& t)->bool { return !t || t->GetStats().GetStat(StatsID::Health) <= 0; }), std::end(s_registry));
}

EntityText* EntityText::CreateTextEntity(const TextEntityDesc& desc) {
    auto* ptr = new EntityText(desc);
    s_registry.emplace_back(ptr);
    return ptr;
}

void EntityText::ClearTextRegistry() noexcept {
    s_registry.clear();
}

const std::vector<std::unique_ptr<EntityText>>& EntityText::GetEntityRegistry() noexcept {
    return s_registry;
}

void EntityText::AddVerts() noexcept {
    /* DO NOTHING */
}

void EntityText::AddVertsForSelf() noexcept {
    /* DO NOTHING */
}