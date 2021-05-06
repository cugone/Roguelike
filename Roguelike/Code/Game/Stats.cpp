#include "Game/Stats.hpp"

#include <type_traits>

StatsID& operator++(StatsID& a) {
    using underlying = std::underlying_type_t<StatsID>;
    a = static_cast<StatsID>(static_cast<underlying>(a) + 1);
    return a;
}

StatsID operator++(StatsID& a, int) {
    auto result = a;
    ++a;
    return result;
}

StatsID& operator--(StatsID& a) {
    using underlying = std::underlying_type_t<StatsID>;
    a = static_cast<StatsID>(static_cast<underlying>(a) - 1);
    return a;
}

StatsID operator--(StatsID& a, int) {
    auto result = a;
    --a;
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

Stats::Stats(const a2de::XMLElement& elem) {
    a2de::DataUtils::ValidateXmlElement(elem, "stats", "", "", "level,health,attack,defense,speed,accuracy,evasion,luck,experience");
    if(auto* xml_level = elem.FirstChildElement("level")) {
        auto id = StatsID::Level;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_level, default_value);
        SetStat(id, value);
    } else {
        auto value = 1L;
        SetStat(StatsID::Level, value);
    }
    if(auto* xml_health = elem.FirstChildElement("health")) {
        auto id = StatsID::Health;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_health, default_value);
        SetStat(id, value);
        SetStat(StatsID::Health_Max, value);
    } else {
        auto value = 1L;
        SetStat(StatsID::Health, value);
        SetStat(StatsID::Health_Max, value);
    }
    if(auto* xml_attack = elem.FirstChildElement("attack")) {
        auto id = StatsID::Attack;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_attack, default_value);
        SetStat(id, value);
    }
    if(auto* xml_defense = elem.FirstChildElement("defense")) {
        auto id = StatsID::Defense;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_defense, default_value);
        SetStat(id, value);
    }
    if(auto* xml_speed = elem.FirstChildElement("speed")) {
        auto id = StatsID::Speed;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_speed, default_value);
        SetStat(id, value);
    }
    if(auto* xml_evasion = elem.FirstChildElement("evasion")) {
        auto id = StatsID::Evasion;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_evasion, default_value);
        SetStat(id, value);
    }
    if(auto* xml_luck = elem.FirstChildElement("luck")) {
        auto id = StatsID::Luck;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_luck, default_value);
        SetStat(id, value);
    } else {
        auto value = 5L;
        SetStat(StatsID::Luck, value);
    }
    if(auto* xml_experience = elem.FirstChildElement("experience")) {
        auto id = StatsID::Experience;
        auto default_value = GetStat(id);
        auto value = a2de::DataUtils::ParseXmlElementText(*xml_experience, default_value);
        SetStat(id, value);
    }
}

Stats::Stats(std::initializer_list<decltype(_stats)::value_type> l) {
    auto id = StatsID::First_;
    for(const auto value : l) {
        if(id < StatsID::Last_) {
            SetStat(id++, value);
        }
    }
    for(; id != StatsID::Last_; ++id) {
        SetStat(id, 0L);
    }
}

auto Stats::GetStat(const StatsID& id) const noexcept -> decltype(_stats)::value_type {
    return _stats[static_cast<std::size_t>(id)];
}

void Stats::SetStat(const StatsID& id, decltype(_stats)::value_type value) noexcept {
    _stats[static_cast<std::size_t>(id)] = value;
}

decltype(Stats::_stats)::value_type Stats::AdjustStat(const StatsID& id, long value) noexcept {
    const auto i = static_cast<std::size_t>(id);
    _stats[i] += static_cast<decltype(_stats)::value_type>(value);
    return _stats[i];
}

decltype(Stats::_stats)::value_type Stats::MultiplyStat(const StatsID& id, long double value) noexcept {
    const auto i = static_cast<std::size_t>(id);
    _stats[i] *= static_cast<decltype(_stats)::value_type>(std::floor(value));
    return _stats[i];
}
