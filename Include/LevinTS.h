#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <priority_queue>
#include <vector>

#define TEMPLATE template <uint32_t HEIGHT, uint32_t WIDTH>
#define CLASS_NAME Witness<HEIGHT, WIDTH>

template <uint32_t HEIGHT, uint32_t WIDTH>
class Witness
{
public:
    Witness() noexcept;

    ~Witness() noexcept;

    auto Expand() noexcept -> void;

private:
    constexpr auto inff = std::numeric_limits<float>::infinity();
    constexpr int dx[4] = {-1, 0, 1, 0};
    constexpr int dy[4] = {0, 1, 0, -1};

    struct Node_t
    {
        uint32_t x, y;
        uint32_t parent_i;
        uint32_t iInOpenList;
        float log2Depth;
        float log2Policy;
        float cost;
    };

private:
    auto openListPush(const Node_t& p_Node) noexcept -> void;

    auto openListPop(uint32_t p_Top_i) noexcept -> void;

    uint32_t action_c[4] = {}; // up right down left
    std::vector<Node_t> nodePool = {{0, 0, -1, 1, -inff, 0.0f, -inff}};
    std::vector<uint32_t> openList = {0, 0};
    uint32_t grid[HEIGHT + 2][WIDTH + 2] = {};

};

TEMPLATE
CLASS_NAME::Witness() noexcept
{
    for (uint32_t i = HEIGHT + 1; i >= 0; --i)
        this->grid[i][0] = this->grid[i][WIDTH + 1] = -1;
    for (uint32_t i = WIDTH + 1; i >= 0; --i)
        this->grid[0][i] = this->grid[HEIGHT + 1][i] = -1;
}

TEMPLATE
CLASS_NAME::~Witness() noexcept
{

}

TEMPLATE
auto CLASS_NAME::Expand() noexcept -> void
{
    uint32_t best_i = this->openList[1];
    this->openListPop(1);
}

TEMPLATE
auto CLASS_NAME::openListPush(const CLASS_NAME::Node_t& p_Node) noexcept -> void
{
    auto& np = this->nodePool;
    auto& ol = this->openList;
    
}

TEMPLATE
auto CLASS_NAME::openListPop(uint32_t p_Top_i, uint32_t p_TopNode_i) noexcept -> void
{
    auto& np = this->nodePool;
    auto& ol = this->openList;
    uint32_t last_i = ol.back();
    Node_t& nd = np[last_i];
    while (p_Top_i != 1) {
        uint32_t l = p_Top_i << 1, r = l | 1;
        np[ol[l]].cost < np[ol[r]].cost ? p_Top_i = l : p_Top_i = r;
        if (nd.cost > np[ol[p_Top_i]].cost) {
            ol[
                np[ol[p_Top_i]].iInOpenList = p_Top_i >> 1
            ] = ol[p_Top_i];
        }
        else break;
    }
    ol[
        nd.iInOpenList = p_Top_i
    ] = ol[last_i];
    ol.pop_back();
}

#undef CLASS_NAME
#undef TEMPLATE
