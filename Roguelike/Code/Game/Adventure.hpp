#pragma once

#include "Engine/Core/DataUtils.hpp"

#include "Game/Map.hpp"

#include <memory>
#include <vector>

class Actor;
class Renderer;

class Adventure {
public:
    Adventure() noexcept = delete;
    Adventure(const Adventure& other) noexcept = default;
    Adventure(Adventure&& other) noexcept = default;
    Adventure& operator=(const Adventure& other) noexcept = default;
    Adventure& operator=(Adventure&& other) noexcept = default;
    ~Adventure() noexcept = default;

    explicit Adventure(const XMLElement& elem) noexcept;

    Actor* player{};

    std::vector<Map>::iterator CurrentMap() const noexcept;
    void NextMap() noexcept;
    void PreviousMap() noexcept;

protected:
private:
    bool LoadFromXml(const XMLElement& elem) noexcept;

    void PlacePlayerNearEntrance() noexcept;
    void PlacePlayerNearExit() noexcept;

    std::string _name{"UNKNOWN ADVENTURE"};
    std::vector<Map> _maps{};
    std::vector<Map>::iterator _current_map_iter{};
};
