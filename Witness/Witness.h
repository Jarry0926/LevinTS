#pragma once
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

class Witness
{
public:
    Witness() noexcept;

    ~Witness() noexcept;

    auto LoadModel(const char* p_FileName) noexcept -> void;

    auto StoreModel(const char* p_FileName) noexcept -> void;

    auto PrintModel() noexcept -> void;

    auto LoadMap(const char* p_FileName) noexcept -> void;
    
    auto Learn() noexcept -> void;

    auto TrainModel(const char* p_ModelFilePath, const char* p_MapFilePath) noexcept -> void;
    
private:
    static constexpr f32_t INF_F32 = std::numeric_limits<f32_t>::infinity();
    static constexpr i32_t DX[4] = {0, 1, 0, -1};
    static constexpr i32_t DY[4] = {-1, 0, 1, 0};
    static constexpr i32_t TOP = 0, RIGHT = 1, BOTTEM = 2, LEFT = 3;
    static constexpr u8_t GET_DIRECTION_FROM[4] = {BOTTEM, LEFT, TOP, RIGHT};
    
    struct node_t
    {
        i32_t x, y;
        u32_t depth;
        f32_t log2Prob; // policy in log_2 space
        f32_t cost;
        u32_t ctx; // from direction, bullet states
    };
    
private:
    inline auto initContextCenter(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void;

    inline auto initContextBorder(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void;

    inline auto initContextCorner(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void;

    // [Called after reaching a goal state]
    inline auto isSolved() noexcept -> bool;

    // [Called after this->isSolved() returns true]
    // Trace every context and update probabilities 
    inline auto updateProb() noexcept -> void;
    
    inline auto iheapPush(f32_t p_Cost, u32_t p_Target_i) noexcept -> void;
    
    inline auto iheapPop(u32_t p_Top_i) noexcept -> void;

    u32_t height, width;
    i32_t* map; // a cell corresponds to a junction at its bottom-right
    f32_t probPool[1024][4] = {};
    std::vector<node_t> nodePool = {{2, 2, 1, 0.0f, 0}};
    std::vector<u32_t> iheap = {0, 0};
    std::priority_queue<u32_t> _iheap;

};
