#include "core/chunk.hpp"

#include <algorithm>
#include <cmath>

namespace Balmung {
namespace {

float terrain_height(int world_x, int world_z, std::uint32_t seed)
{
    const float sx = static_cast<float>(world_x);
    const float sz = static_cast<float>(world_z);
    const float seed_phase = static_cast<float>(seed % 997u) * 0.013f;
    const float rolling = std::sin(sx * 0.055f + seed_phase) * 5.0f;
    const float ridge = std::cos(sz * 0.047f - seed_phase) * 4.0f;
    const float detail = std::sin((sx + sz) * 0.021f) * 2.0f;
    return 48.0f + rolling + ridge + detail;
}

} // namespace

Chunk::Chunk(int cx, int cz, std::uint32_t seed)
    : _cx(cx), _cz(cz), _seed(seed)
{
    generate();
}

VoxelData* Chunk::get_voxel(int x, int y, int z)
{
    const auto it = _index.find({x, y, z});
    return it == _index.end() ? nullptr : &_voxels[it->second];
}

const VoxelData* Chunk::get_voxel(int x, int y, int z) const
{
    const auto it = _index.find({x, y, z});
    return it == _index.end() ? nullptr : &_voxels[it->second];
}

VoxelData* Chunk::place_voxel(int x, int y, int z, std::string block_type)
{
    if (y < WORLD_MIN_Y || y > WORLD_MAX_Y) {
        return nullptr;
    }

    if (auto* existing = get_voxel(x, y, z)) {
        existing->block_type = std::move(block_type);
        return existing;
    }

    _voxels.push_back(VoxelData{x, y, z, std::move(block_type)});
    _index[{x, y, z}] = _voxels.size() - 1;
    return &_voxels.back();
}

bool Chunk::destroy_voxel(int x, int y, int z)
{
    const auto key = std::tuple{x, y, z};
    const auto it = _index.find(key);
    if (it == _index.end()) {
        return false;
    }

    const auto erase_index = it->second;
    const auto last_index = _voxels.size() - 1;
    if (erase_index != last_index) {
        _voxels[erase_index] = std::move(_voxels[last_index]);
        _index[{_voxels[erase_index].x, _voxels[erase_index].y, _voxels[erase_index].z}] = erase_index;
    }

    _voxels.pop_back();
    _index.erase(key);
    return true;
}

void Chunk::generate()
{
    _voxels.clear();
    _index.clear();

    const int base_x = _cx * CHUNK_SIZE;
    const int base_z = _cz * CHUNK_SIZE;
    for (int lx = 0; lx < CHUNK_SIZE; ++lx) {
        for (int lz = 0; lz < CHUNK_SIZE; ++lz) {
            const int x = base_x + lx;
            const int z = base_z + lz;
            const int h = std::clamp(static_cast<int>(terrain_height(x, z, _seed)), 4, WORLD_MAX_Y - 1);
            for (int y = 0; y <= h; ++y) {
                std::string block = "stone";
                if (y == h) {
                    block = h < 50 ? "sand" : "grass";
                } else if (y > h - 4) {
                    block = "dirt";
                }
                _voxels.push_back(VoxelData{x, y, z, std::move(block)});
            }
        }
    }

    rebuild_index();
}

void Chunk::rebuild_index()
{
    _index.clear();
    for (std::size_t i = 0; i < _voxels.size(); ++i) {
        const auto& voxel = _voxels[i];
        _index[{voxel.x, voxel.y, voxel.z}] = i;
    }
}

} // namespace Balmung

