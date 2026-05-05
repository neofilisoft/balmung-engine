#pragma once

#include "core/types.hpp"
#include "core/voxel.hpp"

#include <tuple>
#include <unordered_map>
#include <vector>

namespace Balmung {

constexpr int CHUNK_SIZE = 16;
constexpr int WORLD_MIN_Y = 0;
constexpr int WORLD_MAX_Y = 128;

class Chunk {
public:
    Chunk() = default;
    Chunk(int cx, int cz, std::uint32_t seed);

    [[nodiscard]] int cx() const noexcept { return _cx; }
    [[nodiscard]] int cz() const noexcept { return _cz; }
    [[nodiscard]] const std::vector<VoxelData>& voxels() const noexcept { return _voxels; }

    [[nodiscard]] VoxelData* get_voxel(int x, int y, int z);
    [[nodiscard]] const VoxelData* get_voxel(int x, int y, int z) const;
    VoxelData* place_voxel(int x, int y, int z, std::string block_type);
    bool destroy_voxel(int x, int y, int z);

private:
    int _cx = 0;
    int _cz = 0;
    std::uint32_t _seed = 1337;
    std::vector<VoxelData> _voxels;
    std::unordered_map<std::tuple<int, int, int>, std::size_t, VoxelPosHash> _index;

    void generate();
    void rebuild_index();
};

} // namespace Balmung

