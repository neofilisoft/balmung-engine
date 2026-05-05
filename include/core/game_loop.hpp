#pragma once

#include <algorithm>
#include <chrono>
#include <functional>

namespace Balmung {

class GameLoop {
public:
    using TickFn = std::function<void(float)>;

    void set_fixed_step(float seconds) noexcept { _fixed_step = std::max(seconds, 0.001f); }
    [[nodiscard]] float fixed_step() const noexcept { return _fixed_step; }

    void set_max_accumulator(float seconds) noexcept { _max_accumulator = std::max(seconds, _fixed_step); }
    void reset();
    void tick(float frame_dt, const TickFn& fixed_tick, const TickFn& variable_tick = {});

private:
    float _fixed_step = 1.0f / 60.0f;
    float _max_accumulator = 0.25f;
    float _accumulator = 0.0f;
};

} // namespace Balmung

