#pragma once

#include "Engine/Math/IntVector2.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <vector>

class Pathfinder {
public:

    constexpr static uint8_t PATHFINDING_SUCCESS = 0;
    constexpr static uint8_t PATHFINDING_NO_PATH = 1;
    constexpr static uint8_t PATHFINDING_GOAL_UNREACHABLE = 2;
    constexpr static uint8_t PATHFINDING_INVALID_INITIAL_NODE = 3;

    struct Node {
        std::array<Node*, 8> neighbors{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        Node* parent{nullptr};
        float f = std::numeric_limits<float>::infinity();
        float g = std::numeric_limits<float>::infinity();
        IntVector2 coords = IntVector2::ZERO;
        bool visited = false;
    };

    void Initialize(int width, int height) noexcept;
    const std::vector<const Pathfinder::Node*> GetResult() const noexcept;
    void ResetNavMap() noexcept;

    template<typename Viability, typename Heuristic, typename DistanceFunc>
    uint8_t AStar(const IntVector2& start, const IntVector2& goal, Viability&& viable, Heuristic&& h, DistanceFunc&& distance) {
        auto* initial = GetNode(start);
        if(!initial) {
            return PATHFINDING_INVALID_INITIAL_NODE;
        }
        initial->g = 0.0f;
        initial->f = static_cast<float>(h(start, goal));
        const auto comp = [](const Node* a, const Node* b) { return a->f < b->f;  };
        std::priority_queue<Node*, std::vector<Node*>, decltype(comp)> openSet(comp);
        std::vector<Node*> closedSet{};
        const auto IsGoalInClosedSet = [&closedSet, goal]()->bool {
            const auto found = std::find_if(std::begin(closedSet), std::end(closedSet), [goal](const Node* a)->bool { return a->coords == goal; });
            return found != std::end(closedSet);
        };
        const auto EveryNodeSearched = [this, &closedSet]()->bool {
            return _navMap.size() == closedSet.size();
        };
        const auto IsGoalUnreachable = [&openSet](const Node* current)->bool {
            unsigned char visited_count = 0;
            for(const Node* neighbor : current->neighbors) {
                if(neighbor == nullptr) {
                    ++visited_count;
                    continue;
                }
                if(neighbor->visited) {
                    ++visited_count;
                }
            }
            return visited_count >= 8;
        };
        openSet.push(initial);
        while(!openSet.empty() && !IsGoalInClosedSet()) {
            Node* current = openSet.top();
            if(!openSet.empty() && current->visited) {
                openSet.pop();
            }
            if(openSet.empty()) {
                break;
            }
            current = openSet.top();
            current->visited = true;
            closedSet.push_back(current);
            if(current->coords == goal) {
                _path.push_back(current);
                break;
            }
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
        if(!IsGoalInClosedSet() && _path.empty()) {
            return PATHFINDING_NO_PATH;
        }
        if(!IsGoalInClosedSet() && !_path.empty()) {
            return PATHFINDING_GOAL_UNREACHABLE;
        }
        if(!_path.empty()) {
            const Node* end = _path.front();
            if(end) {
                if(const Node* p = end) {
                    _path.clear();
                    while(p->parent) {
                        _path.push_back(p);
                        p = p->parent;
                    }
                }
            }
            return PATHFINDING_SUCCESS;
        }
        return PATHFINDING_NO_PATH;
    }

    template<typename Viability, typename DistanceFunc>
    uint8_t Dijkstra(const IntVector2& start, const IntVector2& goal, Viability&& viable, DistanceFunc&& distance) {
        return AStar(start, goal, viable, [](const IntVector2&, const IntVector2&) { return 0; }, distance);
    }

protected:
private:
    const Pathfinder::Node* GetNode(int x, int y) const noexcept;
    Pathfinder::Node* GetNode(int x, int y) noexcept;
    const Pathfinder::Node* GetNode(const IntVector2& pos) const noexcept;
    Pathfinder::Node* GetNode(const IntVector2& pos) noexcept;
    const std::array<const Pathfinder::Node*, 8> GetNeighbors(int x, int y) const noexcept;
    void SetNeighbors(int x, int y) noexcept;

    std::vector<const Node*> _path{};
    std::vector<Node> _navMap{};
    IntVector2 _dimensions{};
};
