#pragma once

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVector2.hpp"
#include "Engine/Math/MathUtils.hpp"

#include "Engine/Profiling/ProfileLogScope.hpp"

#include "Game/Map.hpp"
#include "Game/Tile.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <vector>

class Pathfinder {
public:

    struct Node {
        std::array<Node*, 8> neighbors{};
        Node* parent{};
        float f = std::numeric_limits<float>::infinity();
        float g = std::numeric_limits<float>::infinity();
        IntVector2 coords = IntVector2::ZERO;
        bool visited = false;
    };

    void Initialize(int width, int height) noexcept;
    std::vector<Node*> GetResult() noexcept;
    void ResetNavMap() noexcept;

    template<typename Viability, typename Heuristic, typename DistanceFunc>
    void AStar(const IntVector2& start, const IntVector2& goal, Viability&& viable, Heuristic&& h, DistanceFunc&& distance) {
        PROFILE_LOG_SCOPE_FUNCTION();
        auto* initial = GetNode(start);
        if(!initial) {
            return;
        }
        initial->g = 0.0f;
        initial->f = static_cast<float>(h(start, goal));
        auto comp = [](const Node* a, const Node* b) { return a->f < b->f;  };
        std::priority_queue<Node*, std::vector<Node*>, decltype(comp)> openSet(comp);
        openSet.push(initial);
        while(!openSet.empty()) {
            auto* current = openSet.top();
            if(current->coords == goal) {
                _path.push_back(current);
                break;
            }
            if(!openSet.empty() && current->visited) {
                openSet.pop();
            }
            if(openSet.empty()) {
                break;
            }
            current = openSet.top();
            current->visited = true;
            for(const auto neighbor : current->neighbors) {
                if(!neighbor) {
                    continue;
                }
                if(const auto n_idx = neighbor->coords.y * _dimensions.x + neighbor->coords.x; 0 > n_idx || n_idx >= _dimensions.x * _dimensions.y) {
                    continue;
                }
                if(neighbor->visited || !viable(neighbor->coords)) {
                    continue;
                }
                openSet.push(neighbor);
                const float tenativeGScore = current->g + distance(current->coords, neighbor->coords);
                if(tenativeGScore < neighbor->g) {
                    neighbor->parent = current;
                    neighbor->g = tenativeGScore;
                    neighbor->f = neighbor->g + h(neighbor->coords, goal);
                }
            }
        }
        if(!_path.empty()) {
            Node* end = _path.front();
            if(end) {
                if(auto* p = end) {
                    _path.clear();
                    while(p->parent) {
                        _path.push_back(p);
                        p = p->parent;
                    }
                }
            }
        }
    }

    template<typename Viability, typename DistanceFunc>
    void Dijkstra(const IntVector2& start, const IntVector2& goal, Viability&& viable, DistanceFunc&& distance) {
        PROFILE_LOG_SCOPE_FUNCTION();
        AStar(start, goal, viable, [](const IntVector2&, const IntVector2&) { return 0; }, distance);
    }

protected:
private:
    const Pathfinder::Node* GetNode(int x, int y) const noexcept;
    Pathfinder::Node* GetNode(int x, int y) noexcept;
    const Pathfinder::Node* GetNode(const IntVector2& pos) const noexcept;
    Pathfinder::Node* GetNode(const IntVector2& pos) noexcept;
    const std::array<const Pathfinder::Node*, 8> GetNeighbors(int x, int y) const noexcept;
    void SetNeighbors(int x, int y) noexcept;

    std::vector<Node*> _path{};
    std::vector<Node> _navMap{};
    IntVector2 _dimensions{};
};
