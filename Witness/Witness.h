#pragma once
#include <bitset>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <vector>
#include <queue>

#include <General.h>

using i8_t = int8_t;
using i16_t = int16_t;
using i32_t = int32_t;
using u8_t = uint8_t;
using u16_t = uint16_t;
using u32_t = uint32_t;
using f32_t = float;
using f64_t = double;

class Witness
{
public:
    Witness() noexcept;

    ~Witness() noexcept;

    auto LoadModel(const char* p_FileName) noexcept -> void;

    auto StoreModel(const char* p_FileName) noexcept -> void;

    auto PrintModel() noexcept -> void;

    auto LoadMap(const char* p_FileName) noexcept -> void;
    
    auto Learn() noexcept -> bool;

    auto TrainModel(const char* p_ModelFilePath, const char* p_MapFilePath) noexcept -> void;
    
private:
    static constexpr f32_t INF_F32 = std::numeric_limits<f32_t>::infinity();
    static constexpr i32_t DX[4] = {0, 1, 0, -1};
    static constexpr i32_t DY[4] = {-1, 0, 1, 0};
    static constexpr i32_t TOP = 0, RIGHT = 1, BOTTEM = 2, LEFT = 3;
    static constexpr u8_t FROM[4] = {BOTTEM, LEFT, TOP, RIGHT};
    
    struct node_t
    {
        u32_t x, y, p;
        f32_t depth, log2policy, cost;
        u32_t ctx; // from direction, bullet states
        // TODO: optimize it with HLD
        std::bitset<49> vis; // positions visited in previous search of this branch
    };
    
private:
    inline auto genContextCenter(FILE* fd, u8_t* c) noexcept -> void;

    inline auto genContextBorder(FILE* fd, u8_t* c) noexcept -> void;

    inline auto genContextCorner(FILE* fd, u8_t* c) noexcept -> void;

    // [Called after reaching a goal state]
    inline auto isSolved(u32_t i) noexcept -> bool;

    // [Called after this->isSolved() returns true]
    // Trace every context and update policyabilities 
    inline auto updatePolicy(u32_t i) noexcept -> void;
    
    inline auto iheapPush(f32_t v, u32_t i) noexcept -> void;
    
    inline auto iheapPop(u32_t i) noexcept -> void;

    u32_t height, width;
    i32_t* map; // a cell corresponds to a junction at its bottom-right
    f32_t policyPool[1024][4] = {};
    std::vector<node_t> nodePool = {{2, 2, 0, 1.0f, 0.0f, 0.0f}};
    std::vector<u32_t> iheap = {0, 0};

};
