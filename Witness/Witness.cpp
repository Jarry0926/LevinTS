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
    FILE* fd = fopen(p_FileName, "wb+");
    u8_t ctx[18];
    i32_t c = fget(fd);
    if (c == EOF) {
        for (ctx[0] = 0; ctx[0] < 4; ++ctx[0]) {
            this->initContextCenter(fd, ctx);
            this->initContextEdge(fd, ctx);
            this->initContextCorner(fd, ctx);
        }
    }
    else ungetc(c, fd);
    f32_t* base = reinterpret_cast<f32_t*>(&ctx[2]);
    u32_t* ip = reinterpret_cast<u32_t*>(&ctx[0]);
    while (fread(ctx, 18, 1, fd)) {
        u32_t i = *ip >> 16;
        this->probPool[i][0] = base[0];
        this->probPool[i][1] = base[1];
        this->probPool[i][2] = base[2];
        this->probPool[i][3] = base[3];
    }
    fclose(fd);
}

auto Witness::StoreModel(const char* p_FileName) noexcept -> void
{
    FILE* fd = fopen(p_FileName, "wb");
    for (i32_t i = 1023; i >= 0; --i) {
        u8_t bytes[18] = {i >> 8, i | 0xff};
        f32_t* base = reinterpret_cast<f32_t*>(&bytes[3]);
        base[0] = this->probPool[i][0];
        base[1] = this->probPool[i][1];
        base[2] = this->probPool[i][2];
        base[3] = this->probPool[i][3];
        fwrite(bytes, 16, 1, fd);
    }
    fclose(fd);
}

auto Witness::LoadMap(const char* p_FileName) noexcept -> void
{
    FILE* fd = fopen(p_FileName, "r");
    u32_t n;
    fscanf(fd, "%u%u%u", &this->height, &this->width, &n);
    u32_t h = this->height += 3, w = this->width += 3;
    if (this->map != nullptr) {
        delete[] this->map;
        this->map = nullptr;
    }
    i32_t* mp = this->map = new i32_t[w * h];
    for (i32_t i = w - 1; i >= 0; --i)
        mp[i] = mp[(h - 1) * w + i] = 4;
    for (i32_t i = h - 1; i >= 0; --i)
        mp[i * w] = mp[i * w + w - 1] = 4;
    while (n--) {
        u32_t x, y, z;
        fscanf(fd, "%u%u%u", &x, &y, &z);
        mp[(y + 2) * w + x + 2] = z;
    }
#if defined(_DEUBG)
    for (u32_t i = 0; i < this->height; ++i) { 
        for (u32_t j = 0; j < this->width; ++j)
            printf("%u ", this->map[i * this->width + j]);
        printf("\n");
    }
#endif
    fclose(fd);
}

auto Witness::Learn() noexcept -> void
{
    auto& np = this->nodePool;
    auto& ih = this->iheap;
    auto& mp = this->map;
    while (ih.size() > 0) {
        u32_t this_i = ih[1];
        node_t& nd = np[this_i];
        this->iheapPop(1);
        u32_t x = nd.x, y = nd.y;
        // Reach a goal state
        if (x == this->width && y == this->height) {
            if (this->isSolved()) {
                this->updateProb();
                return;
            }
            else continue;
        }
        // Each current node 
        for (u32_t i = 0; i < 4; ++i) {
            u32_t nx = x + DX[i], ny = y + DY[i];
            if (mp[ny][nx] == -1) continue; // skip out of map states
            u32_t _y = ny * this->width;
            u32_t ctx = GET_DIRECTION_FROM[i] << 8
                      | (mp[_y + nx] << 6)
                      | (mp[_y + nx + 1] << 4)
                      | (mp[_y + this->width + nx + 1] << 2)
                      | mp[_y + this->width + nx];
            u32_t newDepth = nd.depth + 1;
            f32_t newLog2Prob = nd.log2Prob + log2f(this->probPool[nd.ctx][i]);
            f32_t newCost = log2f(reinterpret_cast<f32_t>(newDepth)) - newLog2Prob;
            this->iheapPush(newCost, np.size());
            np.emplace_back(nx, ny, newDepth, newLog2Prob, newCost, ctx);
        }
    }
}

auto TrainModel(const char* p_ModelFilePath, const char* p_MapFilePath) noexcept -> void
{
    this->LoadModel(p_ModelFilePath);
    this->LoadMap(p_MapFilePath);
    this->Learn();
    this->StoreModel(p_ModelFilePath);
}

auto Witness::initContextCenter(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void
{
    p_Ctx[1] = 0;
    for (i32_t i = 2; i >= 0; --i) {
        p_Ctx[1] |= i << 6;
        for (i32_t j = 2; j >= 0; --j) {
            p_Ctx[1] |= j << 4;
            for (i32_t k = 2; k >= 0; --k) {
                p_Ctx[1] |= k << 2;
                for (i32_t l = 2; l >= 0; --l) {
                    p_Ctx[1] |= l;
                    f32_t* base = reinterpret_cast<f32_t*>(p_Ctx[2]);
                    base[0] = base[1] = base[2] = base[3] = -log2f(3.0f);
                    base[p_Ctx[0]] = -INF_F32;
                    fwrite(p_Ctx, 18, 1, p_FdModel);
                }
            }
        }
    }
}

auto Witness::initContextEdge(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void
{

}

auto Witness::initContextCorner(FILE* p_FdModel, u8_t* p_Ctx) noexcept -> void
{

}

auto Witness::isSolved() noexcept -> bool
{
    return true;
}

auto Witness::updateProb() noexcept -> void
{

}

auto Witness::iheapPush(f32_t p_Cost, u32_t p_Target_i) noexcept -> void
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

auto Witness::iheapPop(u32_t p_Top_i) noexcept -> void
{
    auto& np = this->nodePool;
    auto& ih = this->iheap;
    u32_t last_v = ih.back();
    ih.pop_back();
    u32_t lastNonLeaf_i = ih.size() << 1;
    Node_t& nd = np[lats_v];
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
