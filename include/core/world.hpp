#pragma once

#include "core/chunk.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Balmung {

class World {
public:
    explicit World(std::uint32_t seed = 1337);

    [[nodiscard]] std::uint32_t seed() const noexcept { return _seed; }
    void set_seed(std::uint32_t seed);

    [[nodiscard]] int loaded_chunk_count() const noexcept { return static_cast<int>(_chunks.size()); }
    void update_chunks(float focus_x, float focus_z, int load_radius = 5);

    [[nodiscard]] Chunk* get_chunk(int cx, int cz);
    [[nodiscard]] const Chunk* get_chunk(int cx, int cz) const;
    [[nodiscard]] VoxelData* get_voxel(int x, int y, int z);
    [[nodiscard]] const VoxelData* get_voxel(int x, int y, int z) const;

    VoxelData* place_voxel(int x, int y, int z, const std::string& block_type);
    bool destroy_voxel(int x, int y, int z);
    void clear();

    template <typename Fn>
    void for_each_voxel(Fn&& fn) const
    {
        for (const auto& [_, chunk] : _chunks) {
            for (const auto& voxel : chunk->voxels()) {
                std::invoke(fn, voxel);
            }
        }
    }

    template <typename Fn>
    void for_each_chunk(Fn&& fn) const
    {
        for (const auto& [_, chunk] : _chunks) {
            std::invoke(fn, *chunk);
        }
    }

private:
    struct ChunkKey {
        int x = 0;
        int z = 0;
        friend bool operator==(const ChunkKey&, const ChunkKey&) = default;
    };

    struct ChunkKeyHash {
        std::size_t operator()(const ChunkKey& key) const noexcept
        {
            const auto x = static_cast<std::uint64_t>(static_cast<std::uint32_t>(key.x));
            const auto z = static_cast<std::uint64_t>(static_cast<std::uint32_t>(key.z));
            return static_cast<std::size_t>(x ^ (z << 32));
        }
    };

    std::uint32_t _seed = 1337;
    std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> _chunks;

    Chunk& ensure_chunk(int cx, int cz);
    static int floor_div(int value, int divisor) noexcept;
};

} // namespace Balmung
