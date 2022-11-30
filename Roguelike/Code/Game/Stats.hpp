#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <array>
#include <utility>

// clang-format off

enum class DamageType {
    None
    , Physical
};

enum class StatsID {
    First_
    , Level = First_
    , Health
    , Health_Max
    , Attack
    , Defense
    , Speed
    , Evasion
    , Luck
    , Experience
    , Max
    , Last_ = Max
};

// clang-format on

StatsID& operator++(StatsID& a);
StatsID operator++(StatsID& a, int);
StatsID& operator--(StatsID& a);
StatsID operator--(StatsID& a, int);

class Stats {
private: //Intentional declaration order for decltype(_stats) to work.
    std::array<long, static_cast<std::size_t>(StatsID::Max)> _stats{};
public:
    Stats() = default;
    explicit Stats(std::initializer_list<decltype(_stats)::value_type> l);
    Stats(const Stats& other) = default;
    Stats(Stats&& r_other) = default;
    Stats& operator=(const Stats& rhs) = default;
    Stats& operator=(Stats&& rrhs) = default;
    ~Stats() = default;

    explicit Stats(const XMLElement& elem);

    constexpr auto GetStat(const StatsID& id) const noexcept -> decltype(_stats)::value_type;
    void SetStat(const StatsID& id, decltype(_stats)::value_type value) noexcept;
    auto AdjustStat(const StatsID& id, long value) noexcept -> decltype(_stats)::value_type;
    auto MultiplyStat(const StatsID& id, long double value) noexcept -> decltype(Stats::_stats)::value_type;

    Stats operator+(const Stats& rhs) const;
    Stats& operator+=(const Stats& rhs);
    Stats operator-(const Stats& rhs) const;
    Stats& operator-=(const Stats& rhs);
    Stats operator-();

protected:
private:

};
