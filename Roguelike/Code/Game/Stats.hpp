#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <array>
#include <utility>

enum class StatsID {
    First_
    ,Level = First_
    ,Health
    ,Attack
    ,Defense
    ,Speed
    ,Experience
    , Max
    , Last_ = Max
};

StatsID& operator++(StatsID& a);
StatsID operator++(StatsID& a, int);
StatsID& operator--(StatsID& a);
StatsID operator--(StatsID& a, int);

class Stats {
private:
    std::array<long long, static_cast<std::size_t>(StatsID::Max)> _stats{};
public:
    Stats() = default;
    Stats(const Stats& other) = default;
    Stats(Stats&& r_other) = default;
    Stats& operator=(const Stats& rhs) = default;
    Stats& operator=(Stats&& rrhs) = default;
    ~Stats() = default;

    Stats(const XMLElement& elem);

    auto GetStat(const StatsID& id) const noexcept -> decltype(_stats)::value_type;
    void SetStat(const StatsID& id, decltype(_stats)::value_type value) noexcept;
    void AdjustStat(const StatsID& id, decltype(_stats)::value_type value) noexcept;
    void MultiplyStat(const StatsID& id, float value) noexcept;

    Stats operator+(const Stats& b) const;
    Stats& operator+=(const Stats& b);
    Stats operator-(const Stats& b) const;
    Stats& operator-=(const Stats& b);
    Stats operator-();
    
protected:
private:

};
