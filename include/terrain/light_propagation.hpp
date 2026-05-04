#pragma once

#include "terrain/chunk_data.hpp"

namespace Balmung {

class LightingSolver {
public:
    void rebuild(ChunkData& chunk) const;
};

} // namespace Balmung


