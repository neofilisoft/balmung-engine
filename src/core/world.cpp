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

void World::update_chunks(float focus_x, float focus_z, int load_radius)
{
    const int center_x = floor_div(static_cast<int>(std::floor(focus_x)), CHUNK_SIZE);
    const int center_z = floor_div(static_cast<int>(std::floor(focus_z)), CHUNK_SIZE);
    load_radius = std::max(load_radius, 1);
    const int unload_radius = load_radius + 2;

    for (int dz = -load_radius; dz <= load_radius; ++dz) {
        for (int dx = -load_radius; dx <= load_radius; ++dx) {
            ensure_chunk(center_x + dx, center_z + dz);
        }
    }

    for (auto it = _chunks.begin(); it != _chunks.end();) {
        const int dx = std::abs(it->first.x - center_x);
        const int dz = std::abs(it->first.z - center_z);
        if (dx > unload_radius || dz > unload_radius) {
            it = _chunks.erase(it);
        } else {
            ++it;
        }
    }
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

