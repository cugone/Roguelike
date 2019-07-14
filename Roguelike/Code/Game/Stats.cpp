#include "Game/Stats.hpp"

#include <type_traits>

StatsID& operator++(StatsID& a) {
    using underlying = std::underlying_type_t<StatsID>;
    a = static_cast<StatsID>(static_cast<underlying>(a) + 1);
    return a;
}

StatsID operator++(StatsID& a, int) {
    auto result = a;
    ++result;
    return result;
}

StatsID& operator--(StatsID& a) {
    using underlying = std::underlying_type_t<StatsID>;
    a = static_cast<StatsID>(static_cast<underlying>(a) - 1);
    return a;
}

StatsID operator--(StatsID& a, int) {
    auto result = a;
    --result;
    return result;
}

Stats Stats::operator+(const Stats& b) const {
    auto result = *this;
    result += b;
    return result;
}

Stats& Stats::operator+=(const Stats& b) {
    for(auto id = StatsID::First_; id != StatsID::Last_; ++id) {
        AdjustStat(id, b.GetStat(id));
    }
    return *this;
}

Stats Stats::operator-(const Stats& b) const {
    auto result = *this;
    result -= b;
    return result;
}

Stats& Stats::operator-=(const Stats& b) {
    for(auto id = StatsID::First_; id != StatsID::Last_; ++id) {
        AdjustStat(id, -b.GetStat(id));
    }
    return *this;
}

Stats Stats::operator-() {
    auto result = *this;
    for(auto id = StatsID::First_; id != StatsID::Last_; ++id) {
        result.SetStat(id, -result.GetStat(id));
    }
    return result;
}

Stats::Stats(const XMLElement& elem) {
    DataUtils::ValidateXmlElement(elem, "stats", "health,attack,defense,speed,evasion,experience", "");
    if(auto* xml_health = elem.FirstChildElement("health")) {
        auto id = StatsID::Health;
        auto default_value = GetStat(id);
        SetStat(id, DataUtils::ParseXmlElementText(*xml_health, default_value));
    }
    if(auto* xml_attack = elem.FirstChildElement("attack")) {
        auto id = StatsID::Attack;
        auto default_value = GetStat(id);
        SetStat(id, DataUtils::ParseXmlElementText(*xml_attack, default_value));
    }
    if(auto* xml_defense = elem.FirstChildElement("defense")) {
        auto id = StatsID::Defense;
        auto default_value = GetStat(id);
        SetStat(id, DataUtils::ParseXmlElementText(*xml_defense, default_value));
    }
    if(auto* xml_speed = elem.FirstChildElement("speed")) {
        auto id = StatsID::Speed;
        auto default_value = GetStat(id);
        SetStat(id, DataUtils::ParseXmlElementText(*xml_speed, default_value));
    }
    if(auto* xml_evasion = elem.FirstChildElement("evasion")) {
        auto id = StatsID::Evasion;
        auto default_value = GetStat(id);
        SetStat(id, DataUtils::ParseXmlElementText(*xml_evasion, default_value));
    }
    if(auto* xml_experience = elem.FirstChildElement("experience")) {
        auto id = StatsID::Experience;
        auto default_value = GetStat(id);
        SetStat(id, DataUtils::ParseXmlElementText(*xml_experience, default_value));
    }
}

auto Stats::GetStat(const StatsID& id) const noexcept -> decltype(_stats)::value_type {
    return _stats[static_cast<std::size_t>(id)];
}

void Stats::SetStat(const StatsID& id, decltype(_stats)::value_type value) noexcept {
    _stats[static_cast<std::size_t>(id)] = value;
}

void Stats::AdjustStat(const StatsID& id, decltype(_stats)::value_type value) noexcept {
    const auto i = static_cast<std::size_t>(id);
    _stats[i] += value;
}

void Stats::MultiplyStat(const StatsID& id, long double value) noexcept {
    const auto i = static_cast<std::size_t>(id);
    _stats[i] *= static_cast<long long>(value);
}
