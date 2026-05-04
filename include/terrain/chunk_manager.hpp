#pragma once

#include "terrain/light_propagation.hpp"
#include "terrain/packed_mesh.hpp"
#include "terrain/voxel_terrain.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Balmung {

class ChunkManager {
public:
    struct Settings {
        int load_radius = 5;
        int unload_radius = 7;
        std::size_t commit_budget = 4;
        std::uint32_t seed = 1337u;
        RenderBackend backend = RenderBackend::Vulkan;
        unsigned worker_count = 0;
    };

    ChunkManager();
    explicit ChunkManager(Settings settings);
    ~ChunkManager();

    void update_focus(float world_x, float world_z);
    void commit_ready(std::size_t budget = 0);

    const ChunkData* find_chunk(const ChunkCoord& coord) const;
    std::shared_ptr<const ChunkData> shared_chunk(const ChunkCoord& coord) const;
    std::optional<PackedChunkMesh> build_mesh_for(const ChunkCoord& coord) const;

    std::size_t loaded_chunk_count() const;
    std::size_t queued_chunk_count() const;
    std::vector<ChunkCoord> loaded_chunks() const;

private:
    Settings settings_;
    TerrainGenerator terrain_;
    LightingSolver lighting_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;

    std::unordered_map<ChunkCoord, std::shared_ptr<ChunkData>, ChunkCoordHash> loaded_;
    std::unordered_set<ChunkCoord, ChunkCoordHash> requested_;
    std::queue<ChunkCoord> jobs_;
    std::queue<std::shared_ptr<ChunkData>> ready_;
    std::vector<std::thread> workers_;

    void worker_loop();
    void request_chunk_locked(const ChunkCoord& coord);
    static int chunk_distance(const ChunkCoord& a, const ChunkCoord& b);
};

} // namespace Balmung


