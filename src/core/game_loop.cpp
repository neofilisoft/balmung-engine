#include "core/game_loop.hpp"

namespace Balmung {

void GameLoop::reset()
{
    _accumulator = 0.0f;
}

void GameLoop::tick(float frame_dt, const TickFn& fixed_tick, const TickFn& variable_tick)
{
    frame_dt = std::clamp(frame_dt, 0.0f, _max_accumulator);
    _accumulator = std::min(_accumulator + frame_dt, _max_accumulator);

    while (_accumulator >= _fixed_step) {
        if (fixed_tick) {
            fixed_tick(_fixed_step);
        }
        _accumulator -= _fixed_step;
    }

    if (variable_tick) {
        variable_tick(frame_dt);
    }
}

} // namespace Balmung

