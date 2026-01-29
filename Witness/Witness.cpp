#include "Witness.h"

Witness::Witness() noexcept
{

}

Witness::~Witness() noexcept
{
    if (this->map != nullptr) {
        delete[] this->map;
        this->map = nullptr;
    }
}

auto Witness::LoadModel(const char* p_FileName) noexcept -> void
{
    FILE* fd = fopen(p_FileName, "rb+");
    u8_t ctx[18];
    i32_t c = fgetc(fd);
    if (c == EOF) {
        for (ctx[0] = TOP; ctx[0] <= LEFT; ++ctx[0]) {
            this->initContextCenter(fd, ctx);
            this->initContextBorder(fd, ctx);
            this->initContextCorner(fd, ctx);
        }
    }
    else ungetc(c, fd);
    while (fread(ctx, 18, 1, fd)) {
        u32_t i = (static_cast<u32_t>(ctx[0]) << 8) + ctx[1];
        memcpy(&this->probPool[i][0], &ctx[2], 4 * sizeof(float));
    }
    fclose(fd);
#if defined(_DEBUG)
    this->PrintModel();
#endif
}

auto Witness::StoreModel(const char* p_FileName) noexcept -> void
{
    FILE* fd = fopen(p_FileName, "wb");
    for (i32_t i = 1023; i >= 0; --i) {
        u8_t bytes[18] = {static_cast<u8_t>(i >> 8), static_cast<u8_t>(i & 0xff)};
        memcpy(&bytes[2], &this->probPool[i][0], 4 * sizeof(float));
        fwrite(bytes, 18, 1, fd);
    }
    fclose(fd);
}

auto Witness::PrintModel() noexcept -> void
{
    for (u32_t i = 0; i < 1024; ++i) {
        f32_t* pp = &this->probPool[i][0];
        if (pp[0] != 0 || pp[1] != 0 || pp[2] != 0 || pp[3] != 0) {
            printf("%u %u %u %u %u %f %f %f %f\n",
                i >> 8, i >> 6 & 3, i >> 4 & 3, i >> 2 & 3, i & 3,
                pp[0], pp[1], pp[2], pp[3]
            );
        }
    }
}

auto Witness::LoadMap(const char* p_FileName) noexcept -> void
{
    FILE* fd = fopen(p_FileName, "r");
    u32_t n; // number of bullets
    fscanf(fd, "%u%u%u", &this->height, &this->width, &n);
    u32_t h = this->height += 3, w = this->width += 3;
    if (this->map != nullptr) {
        delete[] this->map;
        this->map = nullptr;
    }
    i32_t* mp = this->map = new i32_t[w * h];
    // Specify borders
    for (i32_t i = w - 1; i >= 0; --i) {
        mp[i] = mp[(h - 1) * w + i] = 3;
    }
    for (i32_t i = h - 1; i >= 0; --i) {
        mp[i * w] = mp[i * w + w - 1] = 3;
    }
    // Fill bullets
    while (n--) {
        u32_t x, y, z;
        fscanf(fd, "%u%u%u", &x, &y, &z);
        mp[(y + 2) * w + x + 2] = z;
    }
    fclose(fd);
    // Initialize the starting context
    this->nodePool[0].ctx = mp[2 * w + 2] << 2 | 0x00c3;
#if defined(_DEBUG)
    for (u32_t i = 0; i < this->height; ++i) { 
        for (u32_t j = 0; j < this->width; ++j)
            printf("%u ", this->map[i * this->width + j]);
        printf("\n");
    }
#endif
}

auto Witness::Learn() noexcept -> void
{
    auto& np = this->nodePool;
    auto& ih = this->_iheap; //auto& ih = this->iheap;
    auto& mp = this->map;
    ih.push(0); //
    while (ih.size() > 1) {
        u32_t this_i = ih.top(); //u32_t this_i = ih[1];
        node_t& nd = np[this_i];
        ih.pop(); //this->iheapPop(1);
        u32_t x = nd.x, y = nd.y;
        // Reach a goal state
        if (x == this->width - 2 && y == this->height - 2) {
            if (this->isSolved()) {
                this->updateProb();
                return;
            }
            else continue;
        }
        // Expand current node 
        for (u32_t i = 0; i < 4; ++i) {
            i32_t nx = x + DX[i], ny = y + DY[i];
            u32_t offset = ny * this->width + nx;
            if (mp[offset] == 3) continue; // skip out of map states
            u32_t ctx = (GET_DIRECTION_FROM[i] << 8)
                      | (mp[offset] << 6)
                      | (mp[offset + 1] << 4)
                      | (mp[offset + this->width + 1] << 2)
                      |  mp[offset + this->width];
            u32_t newDepth = nd.depth + 1;
            f32_t newLog2Prob = nd.log2Prob + log2f(this->probPool[nd.ctx][i]);
            f32_t newCost = log2f(static_cast<f32_t>(newDepth)) - newLog2Prob;
            ih.push(np.size()); //this->iheapPush(newCost, np.size());
            np.emplace_back(nx, ny, newDepth, newLog2Prob, newCost, ctx);
        }
    }
}

auto Witness::TrainModel(const char* p_ModelFilePath, const char* p_MapFilePath) noexcept -> void
{
    this->LoadModel(p_ModelFilePath);
    this->LoadMap(p_MapFilePath);
    this->Learn();
    this->StoreModel(p_ModelFilePath);
}

inline auto Witness::initContextCenter(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void
{
    for (i32_t i = 2; i >= 0; --i) {
        for (i32_t j = 2; j >= 0; --j) {
            for (i32_t k = 2; k >= 0; --k) {
                for (i32_t l = 2; l >= 0; --l) {
                    p_Ctx[1] = (i << 6) | (j << 4) | (k << 2) | l;
                    f32_t v = -log2f(3.0f), prob[4] = {v, v, v, v};
                    prob[p_Ctx[0]] = -INF_F32;
                    memcpy(&p_Ctx[2], prob, 4 * sizeof(float));
                    fwrite(p_Ctx, 18, 1, p_FdModel);
                }
            }
        }
    }
}

inline auto Witness::initContextBorder(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void
{
    constexpr u8_t wall[4] = {0xf0, 0x3c, 0x0f, 0xc3};
    constexpr u8_t off[4][2] = {{2, 0}, {6, 0}, {6, 4}, {4, 2}};
    for (i32_t k = 3; k >= 0; --k) {
        if (k == p_Ctx[0]) continue;
        for (i32_t i = 2; i >= 0; --i) {
            for (i32_t j = 2; j >= 0; --j) {
                p_Ctx[1] = (i << off[k][0]) | (j << off[k][1]) | wall[k];
                f32_t prob[4] = {-1.0f, -1.0f, -1.0f, -1.0f};
                prob[p_Ctx[0]] = prob[k] = -INF_F32;
                memcpy(&p_Ctx[2], prob, 4 * sizeof(float));
                fwrite(p_Ctx, 18, 1, p_FdModel);
            }
        }
    }
}

inline auto Witness::initContextCorner(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void
{
    constexpr u8_t wall[4] = {0x3f, 0xcf, 0xf3, 0xfc};
    constexpr u8_t from[4][2] = {{1, 2}, {2, 3}, {0, 3}, {0, 1}};
    for (i32_t k = 3; k >= 0; --k) {
        if (from[k][0] == p_Ctx[0] || from[k][1] == p_Ctx[0]) continue;
        for (i32_t i = 2; i >= 0; --i) {
            p_Ctx[1] = i << (3 - k << 1) | wall[k];
            f32_t prob[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            prob[p_Ctx[0]] = prob[from[k][0]] = prob[from[k][1]] = -INF_F32;
            memcpy(&p_Ctx[2], prob, 4 * sizeof(float));
            fwrite(p_Ctx, 18, 1, p_FdModel);
        }
    }
}

inline auto Witness::isSolved() noexcept -> bool
{
    return true;
}

inline auto Witness::updateProb() noexcept -> void
{
    
}

inline auto Witness::iheapPush(f32_t p_Cost, u32_t p_Target_i) noexcept -> void
{
    auto& np = this->nodePool;
    auto& ih = this->iheap;
    u32_t i = ih.size();
    ih.push_back(p_Target_i);
    while (i != 1) {
        u32_t pi = i >> 1;
        if (np[ih[pi]].cost > p_Cost) {
            ih[i] = ih[pi];
            i = pi;
        }
        else break;
    }
    ih[i] = p_Target_i;
}

inline auto Witness::iheapPop(u32_t p_Top_i) noexcept -> void
{
    auto& np = this->nodePool;
    auto& ih = this->iheap;
    u32_t last_v = ih.back();
    ih.pop_back();
    u32_t lastNonLeaf_i = ih.size() << 1;
    node_t& nd = np[last_v];
    while (p_Top_i <= lastNonLeaf_i) {
        u32_t l = p_Top_i << 1, r = l | 1;
        np[ih[l]].cost < np[ih[r]].cost ? p_Top_i = l : p_Top_i = r;
        if (nd.cost > np[ih[p_Top_i]].cost) {
            ih[p_Top_i >> 1] = ih[p_Top_i];
        }
        else break;
    }
    ih[p_Top_i] = last_v;
}
