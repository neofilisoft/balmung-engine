#include "terrain/chunk_manager.hpp"
#include <algorithm>
#include <cmath>

namespace Balmung {

ChunkManager::ChunkManager()
    : ChunkManager(Settings{}) {}

ChunkManager::ChunkManager(Settings settings)
    : settings_(settings),
      terrain_(TerrainSettings{.seed = settings.seed}),
      lighting_() {
    if (settings_.worker_count == 0) {
        const auto hw = std::thread::hardware_concurrency();
        settings_.worker_count = hw > 2 ? std::max(2u, hw / 2u) : 2u;
    }
    workers_.reserve(settings_.worker_count);
    for (unsigned i = 0; i < settings_.worker_count; ++i) {
        workers_.emplace_back([this]() { worker_loop(); });
    }
}

ChunkManager::~ChunkManager() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

int ChunkManager::chunk_distance(const ChunkCoord& a, const ChunkCoord& b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.z - b.z));
}

void ChunkManager::request_chunk_locked(const ChunkCoord& coord) {
    if (loaded_.contains(coord) || requested_.contains(coord)) {
        return;
    }
    requested_.insert(coord);
    jobs_.push(coord);
}

void ChunkManager::update_focus(float world_x, float world_z) {
    const auto focus = world_to_chunk(static_cast<int>(std::floor(world_x)), static_cast<int>(std::floor(world_z)));
    {
        std::lock_guard lock(mutex_);
        for (int dz = -settings_.load_radius; dz <= settings_.load_radius; ++dz) {
            for (int dx = -settings_.load_radius; dx <= settings_.load_radius; ++dx) {
                request_chunk_locked({focus.x + dx, focus.z + dz});
            }
        }

        std::vector<ChunkCoord> to_unload;
        to_unload.reserve(loaded_.size());
        for (const auto& [coord, _chunk] : loaded_) {
            if (chunk_distance(coord, focus) > settings_.unload_radius) {
                to_unload.push_back(coord);
            }
        }
        for (const auto& coord : to_unload) {
            loaded_.erase(coord);
        }
    }
    cv_.notify_all();
}

void ChunkManager::commit_ready(std::size_t budget) {
    const auto max_to_commit = budget == 0 ? settings_.commit_budget : budget;
    std::lock_guard lock(mutex_);
    for (std::size_t i = 0; i < max_to_commit && !ready_.empty(); ++i) {
        auto chunk = ready_.front();
        ready_.pop();
        requested_.erase(chunk->coord());
        loaded_[chunk->coord()] = std::move(chunk);
    }
}

const ChunkData* ChunkManager::find_chunk(const ChunkCoord& coord) const {
    std::lock_guard lock(mutex_);
    const auto it = loaded_.find(coord);
    return it == loaded_.end() ? nullptr : it->second.get();
}

std::shared_ptr<const ChunkData> ChunkManager::shared_chunk(const ChunkCoord& coord) const {
    std::lock_guard lock(mutex_);
    const auto it = loaded_.find(coord);
    return it == loaded_.end() ? nullptr : it->second;
}

std::optional<PackedChunkMesh> ChunkManager::build_mesh_for(const ChunkCoord& coord) const {
    auto self = shared_chunk(coord);
    if (!self) {
        return std::nullopt;
    }
    ChunkNeighborhood neighborhood{};
    neighborhood.self = self.get();
    neighborhood.neg_x = find_chunk({coord.x - 1, coord.z});
    neighborhood.pos_x = find_chunk({coord.x + 1, coord.z});
    neighborhood.neg_z = find_chunk({coord.x, coord.z - 1});
    neighborhood.pos_z = find_chunk({coord.x, coord.z + 1});
    ChunkMesher mesher;
    return mesher.build(neighborhood, settings_.backend);
}

std::size_t ChunkManager::loaded_chunk_count() const {
    std::lock_guard lock(mutex_);
    return loaded_.size();
}

std::size_t ChunkManager::queued_chunk_count() const {
    std::lock_guard lock(mutex_);
    return jobs_.size() + ready_.size();
}

std::vector<ChunkCoord> ChunkManager::loaded_chunks() const {
    std::lock_guard lock(mutex_);
    std::vector<ChunkCoord> coords;
    coords.reserve(loaded_.size());
    for (const auto& [coord, _chunk] : loaded_) {
        coords.push_back(coord);
    }
    return coords;
}

void ChunkManager::worker_loop() {
    while (true) {
        ChunkCoord coord{};
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this]() { return stop_ || !jobs_.empty(); });
            if (stop_) {
                return;
            }
            coord = jobs_.front();
            jobs_.pop();
        }

        auto chunk = std::make_shared<ChunkData>(coord);
        terrain_.generate_chunk(*chunk, nullptr);
        lighting_.rebuild(*chunk);

        {
            std::lock_guard lock(mutex_);
            ready_.push(std::move(chunk));
        }
    }
}

} // namespace Balmung


