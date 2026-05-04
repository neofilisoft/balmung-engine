#include "engine.hpp"

#include <chrono>
#include <thread>

int main()
{
    Balmung::EngineConfig config;
    config.headless = false;
    config.viewport_w = 1280;
    config.viewport_h = 720;

    Balmung::Engine engine(config);
    if (!engine.init()) {
        return 1;
    }

    auto last = std::chrono::steady_clock::now();
    while (engine.running()) {
        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float>(now - last).count();
        last = now;
        engine.tick(dt);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

