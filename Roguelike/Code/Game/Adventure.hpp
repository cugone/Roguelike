#pragma once

#include "Engine/Core/DataUtils.hpp"

#include <memory>
#include <vector>

class Map;
class Renderer;

class Adventure {
public:
    Adventure() noexcept = delete;
    Adventure(const Adventure& other) noexcept = default;
    Adventure(Adventure&& other) noexcept = default;
    Adventure& operator=(const Adventure& other) noexcept = default;
    Adventure& operator=(Adventure&& other) noexcept = default;
    ~Adventure() noexcept = default;

    explicit Adventure(Renderer& renderer, const XMLElement& elem) noexcept;

    Map* currentMap{};

    void NextMap() noexcept;
    void PreviousMap() noexcept;

protected:
private:
    bool LoadFromXml(const XMLElement& elem) noexcept;
    void PlacePlayerNearEntrance() noexcept;
    void PlacePlayerNearExit() noexcept;

    Renderer& _renderer;
    std::string _name{"UNKNOWN ADVENTURE"};
    std::vector<std::unique_ptr<class Map>> _maps{};
    decltype(_maps)::iterator _current_map_iter{};
};
