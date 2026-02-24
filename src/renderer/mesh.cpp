#if __has_include(<glad/glad.h>)
#  include <glad/glad.h>
#  define VB_HAS_GL 1
#else
#  define VB_HAS_GL 0
#endif

#include "renderer/mesh.hpp"
#include "core/chunk.hpp"
#include "core/voxel.hpp"
#include <cstddef>
#include <unordered_map>
#include <tuple>

namespace VoxelBlock {

// ── Mesh ──────────────────────────────────────────────────────────────────────

Mesh::~Mesh() {
#if VB_HAS_GL
    if (_vao) { glDeleteVertexArrays(1, &_vao); glDeleteBuffers(1, &_vbo); glDeleteBuffers(1, &_ebo); }
#endif
}

Mesh::Mesh(Mesh&& o) noexcept
    : _vao(o._vao), _vbo(o._vbo), _ebo(o._ebo), _index_count(o._index_count)
{ o._vao=o._vbo=o._ebo=0; o._index_count=0; }

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this!=&o) {
        this->~Mesh(); _vao=o._vao; _vbo=o._vbo; _ebo=o._ebo; _index_count=o._index_count;
        o._vao=o._vbo=o._ebo=0; o._index_count=0;
    }
    return *this;
}

void Mesh::upload(const std::vector<Vertex>& verts, const std::vector<uint32_t>& idxs) {
    _index_count = (int)idxs.size();
#if VB_HAS_GL
    if (!_vao) {
        glGenVertexArrays(1, &_vao);
        glGenBuffers(1, &_vbo);
        glGenBuffers(1, &_ebo);
    }
    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxs.size()*sizeof(uint32_t), idxs.data(), GL_STATIC_DRAW);

    constexpr int S = sizeof(Vertex);
    // attrib 0: position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, S, (void*)offsetof(Vertex, px));
    glEnableVertexAttribArray(0);
    // attrib 1: normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, S, (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(1);
    // attrib 2: color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, S, (void*)offsetof(Vertex, cr));
    glEnableVertexAttribArray(2);
    // attrib 3: uv
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, S, (void*)offsetof(Vertex, u));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
#endif
}

void Mesh::update(const std::vector<Vertex>& v, const std::vector<uint32_t>& i) {
    upload(v, i);
}

void Mesh::draw() const {
#if VB_HAS_GL
    if (!_vao || !_index_count) return;
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
#endif
}

// ── Chunk Mesh Builder ────────────────────────────────────────────────────────

namespace {

struct VoxelLookup {
    std::unordered_map<std::tuple<int,int,int>, std::string, VoxelPosHash> map;
    VoxelLookup() = default;
    explicit VoxelLookup(const Chunk* c) {
        if (!c) return;
        for (auto& v : c->voxels())
            map[{v.x, v.y, v.z}] = v.block_type;
    }
    bool solid(int x, int y, int z) const {
        auto it = map.find({x,y,z});
        if (it == map.end()) return false;
        const auto* def = BlockRegistry::get_block(it->second);
        return def && def->has_tag("solid");
    }
};

// Face quad helper: adds 4 vertices + 2 triangles
void add_face(ChunkMeshData& out,
              std::array<float,3> p0, std::array<float,3> p1,
              std::array<float,3> p2, std::array<float,3> p3,
              std::array<float,3> normal, std::array<float,3> col)
{
    uint32_t base = (uint32_t)out.vertices.size();
    // UVs for the 4 corners
    float uvs[4][2] = {{0,0},{1,0},{1,1},{0,1}};
    std::array<std::array<float,3>,4> pts = {p0,p1,p2,p3};
    for (int i=0; i<4; ++i) {
        out.vertices.push_back({
            pts[i][0], pts[i][1], pts[i][2],
            normal[0], normal[1], normal[2],
            col[0], col[1], col[2],
            uvs[i][0], uvs[i][1]
        });
    }
    // Two triangles: 0,1,2  0,2,3
    out.indices.push_back(base+0); out.indices.push_back(base+1); out.indices.push_back(base+2);
    out.indices.push_back(base+0); out.indices.push_back(base+2); out.indices.push_back(base+3);
}

} // anon ns

ChunkMeshData build_chunk_mesh(const Chunk& chunk,
                               const Chunk* n_nx, const Chunk* n_px,
                               const Chunk* n_nz, const Chunk* n_pz)
{
    ChunkMeshData out;

    VoxelLookup self(&chunk);
    VoxelLookup nx_lkp(n_nx), px_lkp(n_px), nz_lkp(n_nz), pz_lkp(n_pz);

    auto is_solid = [&](int x, int y, int z) -> bool {
        if (self.solid(x,y,z)) return true;
        if (x < chunk.cx()*CHUNK_SIZE)         return nx_lkp.solid(x,y,z);
        if (x >= (chunk.cx()+1)*CHUNK_SIZE)     return px_lkp.solid(x,y,z);
        if (z < chunk.cz()*CHUNK_SIZE)          return nz_lkp.solid(x,y,z);
        if (z >= (chunk.cz()+1)*CHUNK_SIZE)     return pz_lkp.solid(x,y,z);
        return false;
    };

    for (auto& vox : chunk.voxels()) {
        const auto* def = BlockRegistry::get_block(vox.block_type);
        if (!def) continue;
        if (!def->has_tag("solid")) continue;   // water / non-solid: skip for now

        float r = def->color.r / 255.f;
        float g = def->color.g / 255.f;
        float b = def->color.b / 255.f;
        std::array<float,3> col = {r,g,b};

        float x = (float)vox.x, y = (float)vox.y, z = (float)vox.z;

        // +Y face (top)
        if (!is_solid(vox.x, vox.y+1, vox.z)) {
            float lit = 1.0f;  // top face brightest
            std::array<float,3> c = {col[0]*lit, col[1]*lit, col[2]*lit};
            add_face(out,
                {x,y+1,z}, {x+1,y+1,z}, {x+1,y+1,z+1}, {x,y+1,z+1},
                {0,1,0}, c);
        }
        // -Y face (bottom)
        if (!is_solid(vox.x, vox.y-1, vox.z)) {
            std::array<float,3> c = {col[0]*.4f, col[1]*.4f, col[2]*.4f};
            add_face(out,
                {x,y,z+1}, {x+1,y,z+1}, {x+1,y,z}, {x,y,z},
                {0,-1,0}, c);
        }
        // +X face
        if (!is_solid(vox.x+1, vox.y, vox.z)) {
            std::array<float,3> c = {col[0]*.7f, col[1]*.7f, col[2]*.7f};
            add_face(out,
                {x+1,y,z}, {x+1,y,z+1}, {x+1,y+1,z+1}, {x+1,y+1,z},
                {1,0,0}, c);
        }
        // -X face
        if (!is_solid(vox.x-1, vox.y, vox.z)) {
            std::array<float,3> c = {col[0]*.7f, col[1]*.7f, col[2]*.7f};
            add_face(out,
                {x,y,z+1}, {x,y,z}, {x,y+1,z}, {x,y+1,z+1},
                {-1,0,0}, c);
        }
        // +Z face
        if (!is_solid(vox.x, vox.y, vox.z+1)) {
            std::array<float,3> c = {col[0]*.85f, col[1]*.85f, col[2]*.85f};
            add_face(out,
                {x,y,z+1}, {x+1,y,z+1}, {x+1,y+1,z+1}, {x,y+1,z+1},
                {0,0,1}, c);
        }
        // -Z face
        if (!is_solid(vox.x, vox.y, vox.z-1)) {
            std::array<float,3> c = {col[0]*.85f, col[1]*.85f, col[2]*.85f};
            add_face(out,
                {x+1,y,z}, {x,y,z}, {x,y+1,z}, {x+1,y+1,z},
                {0,0,-1}, c);
        }
    }
    return out;
}

} // namespace VoxelBlock
