#include "core/world.hpp"

#include "core/event_system.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Balmung {

World::World(std::uint32_t seed)
    : _seed(seed)
{
}

void World::set_seed(std::uint32_t seed)
{
    _seed = seed == 0 ? 1337 : seed;
    clear();
}

void World::set_streaming_settings(StreamingSettings settings) noexcept
{
    settings.min_load_radius = std::max(settings.min_load_radius, 1);
    settings.load_radius = std::max(settings.load_radius, settings.min_load_radius);
    settings.unload_margin = std::max(settings.unload_margin, 0);
    _streaming = settings;
}

void World::update_chunks(float focus_x, float focus_z, int load_radius)
{
    const int center_x = floor_div(static_cast<int>(std::floor(focus_x)), CHUNK_SIZE);
    const int center_z = floor_div(static_cast<int>(std::floor(focus_z)), CHUNK_SIZE);
    const int effective_load_radius = load_radius > 0
        ? std::max(load_radius, _streaming.min_load_radius)
        : std::max(_streaming.load_radius, _streaming.min_load_radius);
    const int unload_radius = effective_load_radius + _streaming.unload_margin;

    _streaming_stats.center_chunk_x = center_x;
    _streaming_stats.center_chunk_z = center_z;
    _streaming_stats.chunks_loaded_this_update = 0;
    _streaming_stats.chunks_unloaded_this_update = 0;

    for (int dz = -effective_load_radius; dz <= effective_load_radius; ++dz) {
        for (int dx = -effective_load_radius; dx <= effective_load_radius; ++dx) {
            const auto before = _chunks.size();
            ensure_chunk(center_x + dx, center_z + dz);
            if (_chunks.size() > before) {
                ++_streaming_stats.chunks_loaded_this_update;
            }
        }
    }

    if (_streaming.seamless_streaming) {
        for (auto it = _chunks.begin(); it != _chunks.end();) {
            const int dx = std::abs(it->first.x - center_x);
            const int dz = std::abs(it->first.z - center_z);
            if (dx > unload_radius || dz > unload_radius) {
                it = _chunks.erase(it);
                ++_streaming_stats.chunks_unloaded_this_update;
            } else {
                ++it;
            }
        }
    }

    _streaming_stats.loaded_chunks = loaded_chunk_count();
}

Chunk* World::get_chunk(int cx, int cz)
{
    const auto it = _chunks.find(ChunkKey{cx, cz});
    return it == _chunks.end() ? nullptr : it->second.get();
}

const Chunk* World::get_chunk(int cx, int cz) const
{
    const auto it = _chunks.find(ChunkKey{cx, cz});
    return it == _chunks.end() ? nullptr : it->second.get();
}

VoxelData* World::get_voxel(int x, int y, int z)
{
    const int cx = floor_div(x, CHUNK_SIZE);
    const int cz = floor_div(z, CHUNK_SIZE);
    auto* chunk = get_chunk(cx, cz);
    return chunk ? chunk->get_voxel(x, y, z) : nullptr;
}

const VoxelData* World::get_voxel(int x, int y, int z) const
{
    const int cx = floor_div(x, CHUNK_SIZE);
    const int cz = floor_div(z, CHUNK_SIZE);
    const auto* chunk = get_chunk(cx, cz);
    return chunk ? chunk->get_voxel(x, y, z) : nullptr;
}

VoxelData* World::place_voxel(int x, int y, int z, const std::string& block_type)
{
    auto& chunk = ensure_chunk(floor_div(x, CHUNK_SIZE), floor_div(z, CHUNK_SIZE));
    auto* voxel = chunk.place_voxel(x, y, z, block_type);
    if (voxel) {
        EventData data;
        data.set("x", x);
        data.set("y", y);
        data.set("z", z);
        data.set("block", block_type);
        EventSystem::emit("on_place", data);
    }
    return voxel;
}

bool World::destroy_voxel(int x, int y, int z)
{
    auto* chunk = get_chunk(floor_div(x, CHUNK_SIZE), floor_div(z, CHUNK_SIZE));
    if (!chunk || !chunk->destroy_voxel(x, y, z)) {
        return false;
    }

    EventData data;
    data.set("x", x);
    data.set("y", y);
    data.set("z", z);
    EventSystem::emit("on_destroy", data);
    return true;
}

void World::clear()
{
    _chunks.clear();
    _streaming_stats.loaded_chunks = 0;
}

Chunk& World::ensure_chunk(int cx, int cz)
{
    const ChunkKey key{cx, cz};
    auto it = _chunks.find(key);
    if (it != _chunks.end()) {
        return *it->second;
    }

    auto chunk = std::make_unique<Chunk>(cx, cz, _seed);
    auto* raw = chunk.get();
    _chunks.emplace(key, std::move(chunk));
    return *raw;
}

int World::floor_div(int value, int divisor) noexcept
{
    int q = value / divisor;
    int r = value % divisor;
    if (r != 0 && ((r < 0) != (divisor < 0))) {
        --q;
    }
    return q;
}

} // namespace Balmung
