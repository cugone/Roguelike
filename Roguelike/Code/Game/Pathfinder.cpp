#include "Game/Pathfinder.hpp"

void Pathfinder::Initialize(int width, int height) noexcept {
    static bool already_initialized{false};
    if(already_initialized) {
        return;
    }
    _dimensions = IntVector2{width, height};
    const auto area = width * height;
    _path.clear();
    _navMap.resize(area);
    for(auto x = 0; x < width; ++x) {
        for(auto y = 0; y < height; ++y) {
            auto& node = _navMap[static_cast<std::size_t>(y) * width + x];
            node.coords = IntVector2{x, y};
            SetNeighbors(x, y);
        }
    }
    already_initialized = true;
}

const std::vector<const Pathfinder::Node*> Pathfinder::GetResult() const noexcept {
    return {std::crbegin(_path), std::crend(_path)};
}

void Pathfinder::ResetNavMap() noexcept {
    for(auto& node : _navMap) {
        node = Node{};
    }
    Initialize(_dimensions.x, _dimensions.y);
}

const Pathfinder::Node* Pathfinder::GetNode(int x, int y) const noexcept {
    return GetNode(IntVector2{x, y});
}

Pathfinder::Node* Pathfinder::GetNode(int x, int y) noexcept {
    return GetNode(IntVector2{x, y});
}

const Pathfinder::Node* Pathfinder::GetNode(const IntVector2& pos) const noexcept {
    if(pos.x < 0 || pos.y < 0) {
        return nullptr;
    }
    const auto index = static_cast<std::size_t>(static_cast<std::size_t>(pos.y) * _dimensions.y + pos.x);
    if(index >= _navMap.size()) return nullptr;
    return &_navMap[index];
}

Pathfinder::Node* Pathfinder::GetNode(const IntVector2& pos) noexcept {
    return const_cast<Pathfinder::Node*>(static_cast<const Pathfinder&>(*this).GetNode(pos));
}

const std::array<const Pathfinder::Node*, 8> Pathfinder::GetNeighbors(int x, int y) const noexcept {
    return {GetNode(x - 1, y - 1), GetNode(x - 0, y - 1), GetNode(x + 1, y - 1)
           , GetNode(x + 1, y + 0), GetNode(x + 1, y + 1), GetNode(x + 0, y + 1)
           , GetNode(x - 1, y + 1), GetNode(x - 1, y + 0)};
}

void Pathfinder::SetNeighbors(int x, int y) noexcept {
    auto* node = GetNode(x, y);
    if(!node) {
        return;
    }
    const auto& neighbors = GetNeighbors(x, y);
    for(int i = 0; i < neighbors.size(); ++i) {
        if(x == 0 && y == 0) {
            if(i < 3 || i >= 6) continue;
        }
        if(x == _dimensions.x - 1 && y == 0) {
            if(i < 5) continue;
        }
        if(x == _dimensions.x - 1 && y == _dimensions.y - 1) {
            if(i < 6 && i >= 2) continue;
        }
        if(x == 0 && y == _dimensions.y - 1) {
            if(i == 0 || i >= 4) continue;
        }
        if(x == 0 || x == _dimensions.x - 1) {
            if(x == 0 && i == 0 || i >= 6) continue;
            if(x == _dimensions.x - 1 && i >= 2 && i <= 4) continue;
        }
        if(y == 0 || x == _dimensions.y - 1) {
            if(y == 0 && i >= 0 || i <= 2) continue;
            if(y == _dimensions.y - 1 && i >= 4 && i <= 6) continue;
        }
        auto neighbor_x = x;
        auto neighbor_y = y;
        switch(i) {
        case 0:
            --neighbor_x;
            --neighbor_y;
            break;
        case 1:
            --neighbor_y;
            break;
        case 2:
            ++neighbor_x;
            --neighbor_y;
            break;
        case 3:
            ++neighbor_x;
            break;
        case 4:
            ++neighbor_x;
            ++neighbor_y;
            break;
        case 5:
            ++neighbor_y;
            break;
        case 6:
            --neighbor_x;
            ++neighbor_y;
            break;
        case 7:
            --neighbor_x;
            break;
        }
        node->neighbors[i] = GetNode(neighbor_x, neighbor_y);
    }
}
