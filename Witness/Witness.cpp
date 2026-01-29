#include "Witness.h"

Witness::Witness() noexcept
{

}

Witness::~Witness() noexcept
{
    if (this->map != nullptr) delete[] this->map, this->map = nullptr;
}

auto Witness::LoadModel(const char* p_FileName) noexcept -> void
{
    FILE* fd = fopen(p_FileName, "rb+");
    u8_t ctx[18]; // read once each 18B into ctx
    // Generate context if model is an empty file
    i32_t c = fgetc(fd);
    if (c == EOF) {
        for (ctx[0] = TOP; ctx[0] <= LEFT; ++ctx[0]) {
            this->genContextCenter(fd, ctx);
            this->genContextBorder(fd, ctx);
            this->genContextCorner(fd, ctx);
        }
    } else ungetc(c, fd);
    // Read model into this->policyPool
    while (fread(ctx, 18, 1, fd)) {
        u32_t i = (static_cast<u32_t>(ctx[0]) << 8) + ctx[1];
        memcpy(&this->policyPool[i][0], &ctx[2], 4 * sizeof(float));
    } fclose(fd);
#if defined(_DEBUG)
    this->PrintModel();
#endif
}

auto Witness::StoreModel(const char* p_FileName) noexcept -> void
{
    FILE* fd = fopen(p_FileName, "wb");
    for (i32_t i = 1023; i >= 0; --i) {
        u8_t bytes[18] = {static_cast<u8_t>(i >> 8), static_cast<u8_t>(i & 0xff)};
        memcpy(&bytes[2], &this->policyPool[i][0], 4 * sizeof(float));
        fwrite(bytes, 18, 1, fd);
    } fclose(fd);
}

auto Witness::PrintModel() noexcept -> void
{
    for (u32_t i = 0; i < 1024; ++i) {
        f32_t* pp = &this->policyPool[i][0];
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
    if (this->map != nullptr) delete[] this->map, this->map = nullptr;
    i32_t* mp = this->map = new i32_t[w * h];
    // Specify borders
    for (i32_t i = w - 1; i >= 0; --i) mp[i] = mp[(h - 1) * w + i] = 3;
    for (i32_t i = h - 1; i >= 0; --i) mp[i * w] = mp[i * w + w - 1] = 3;
    // Fill bullets
    while (n--) {
        u32_t x, y, z;
        fscanf(fd, "%u%u%u", &x, &y, &z);
        mp[(y + 2) * w + x + 2] = z;
    } fclose(fd);
    // Initialize the starting context
    this->nodePool[0].ctx = mp[2 * w + 2] << 2 | 0x00c3;
    this->nodePool[0].vis[2 * w + 2] = 1;
#if defined(_DEBUG)
    for (u32_t i = 0; i < this->height; ++i) { 
        for (u32_t j = 0; j < this->width; ++j)
            printf("%u ", this->map[i * this->width + j]);
        printf("\n");
    }
#endif
}

auto Witness::Learn() noexcept -> bool
{
    auto& np = this->nodePool;
    auto& ih = this->_iheap; //auto& ih = this->iheap;
    i8_t* mp = this->map;
    i32_t h = this->height, w = this->width;
    ih.push(0); //
    while (ih.size() > 1) {
        auto [x, y, d, lp, c, ctx, vis] = np[ih.top()]; ih.pop(); //auto [x, y, d, lp, c, ctx] = np[ih[1]]; this->iheapPop(1);
        // Reach a goal state
        if (x == w - 2 && y == h - 2) {
            bool s = this->isSolved();
            if (s) this->updatePolicy();
            return s;
        }
        // Expand current node
        for (u32_t i = 0; i < 4; ++i) {
            u32_t nx = x + DX[i], ny = y + DY[i]; // position of next step
            u32_t j = ny * w + nx; // cached for two dimensional index computation
            if (mp[j] == 3 || vis[j]) continue; // skip out of map and visited states
            auto nvis = vis; nvis[j] = 1;
            // handle visited cases
            u32_t m[4] = {mp[j], mp[j + 1], mp[j + w + 1], mp[j + w]};
            if (vis[j - w]) m[0] = m[1] = 3;
            if (vis[j + 1]) m[1] = m[2] = 3;
            if (vis[j + w]) m[2] = m[3] = 3;
            if (vis[j - 1]) m[3] = m[0] = 3;
            
            u32_t nctx = FROM[i] << 8 | m[0] << 6 | m[1] << 4 | m[2] << 2 | m[3];
            f32_t nd = d + 1.0f, nlp = lp + log2f(this->policyPool[ctx][i]), nc = log2f(nd) - nlp;
            ih.push(np.size()); //this->iheapPush(newCost, np.size());
            np.emplace_back(nx, ny, nd, nlp, nc, nctx, std::move(nvis));
        }
    } return false;
}

auto Witness::TrainModel(const char* p_ModelFilePath, const char* p_MapFilePath) noexcept -> void
{
    this->LoadModel(p_ModelFilePath);
    this->LoadMap(p_MapFilePath);
    this->Learn();
    this->StoreModel(p_ModelFilePath);
}

#define FOR(X) for (i32_t X = 2; X >= 0; --X)

inline auto Witness::genContextCenter(FILE* fd, u8_t* c) noexcept -> void
{
    // fd: File descriptor of the model
    // c: context as source of fwrite
    FOR(i) FOR(j) FOR(k) FOR(l) {
        c[1] = i << 6 | j << 4 | k << 2 | l;
        f32_t v = -log2f(3.0f), p[4] = {v, v, v, v}; // log_{2}(policy)
        p[c[0]] = -INF_F32;
        memcpy(&c[2], p, 4 * sizeof(float));
        fwrite(c, 18, 1, fd);
    }
}

inline auto Witness::genContextBorder(FILE* fd, u8_t* c) noexcept -> void
{
    // fd: File descriptor of the model
    // c: context as source of fwrite
    constexpr u8_t wall[4] = {0xf0, 0x3c, 0x0f, 0xc3};
    constexpr u8_t off[4][2] = {{2, 0}, {6, 0}, {6, 4}, {4, 2}};
    for (i32_t k = 3; k >= 0; --k) { // loop through direction against wall
        if (k == c[0]) continue;
        FOR(i) FOR(j) {
            c[1] = i << off[k][0] | j << off[k][1] | wall[k];
            f32_t p[4] = {-1.0f, -1.0f, -1.0f, -1.0f}; // log_{2}(policy)
            p[c[0]] = p[k] = -INF_F32;
            memcpy(&c[2], p, 4 * sizeof(float));
            fwrite(c, 18, 1, fd);
        }
    }
}

inline auto Witness::genContextCorner(FILE* fd, u8_t* c) noexcept -> void
{
    // fd: File descriptor of the model
    // c: context as source of fwrite
    constexpr u8_t wall[4] = {0x3f, 0xcf, 0xf3, 0xfc};
    constexpr u8_t from[4][2] = {{1, 2}, {2, 3}, {0, 3}, {0, 1}};
    for (i32_t k = 3; k >= 0; --k) { // loop through available cell
        if (from[k][0] == c[0] || from[k][1] == c[0]) continue;
        FOR(i) {
            c[1] = i << (3 - k << 1) | wall[k];
            f32_t p[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // log_{2}(policy)
            p[c[0]] = p[from[k][0]] = p[from[k][1]] = -INF_F32;
            memcpy(&c[2], p, 4 * sizeof(float));
            fwrite(c, 18, 1, fd);
        }
    }
}

#undef FOR

inline auto Witness::isSolved() noexcept -> bool
{
    return true;
}

inline auto Witness::updatePolicy() noexcept -> void
{
    
}

inline auto Witness::iheapPush(f32_t v, u32_t i) noexcept -> void
{
    auto& np = this->nodePool;
    auto& ih = this->iheap;
    u32_t j = ih.size(); ih.push_back(i);
    while (j != 1) {
        u32_t pi = j >> 1;
        if (np[ih[pi]].cost > v) ih[j] = ih[pi], j = pi;
        else break;
    } ih[j] = i;
}

inline auto Witness::iheapPop(u32_t i) noexcept -> void
{
    auto& np = this->nodePool;
    auto& ih = this->iheap;
    u32_t v = ih.back(); // value in the last node of heap
    ih.pop_back();
    u32_t j = ih.size() << 1; // index of last non-leaf node in heap
    f32_t c = np[v].cost;
    while (i <= j) {
        u32_t l = i << 1, r = l | 1;
        np[ih[l]].cost < np[ih[r]].cost ? i = l : i = r;
        if (c > np[ih[i]].cost) ih[i >> 1] = ih[i];
        else break;
    } ih[i] = v;
}
