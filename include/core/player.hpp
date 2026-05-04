#pragma once

#include "core/inventory.hpp"
#include "core/types.hpp"

namespace Balmung {

class World;

class Player {
public:
    enum class InputAction {
        PlaceBlock,
        BreakBlock,
        ScrollUp,
        ScrollDown,
    };

    explicit Player(Inventory* inventory = nullptr);

    void set_position(Vec3 position) noexcept { _position = position; }
    [[nodiscard]] Vec3 position() const noexcept { return _position; }

    void set_inventory_open(bool open) noexcept { _inventory_open = open; }
    [[nodiscard]] bool inventory_open() const noexcept { return _inventory_open; }

    void set_block_index(int index) noexcept;
    [[nodiscard]] int block_index() const noexcept { return _block_index; }

    void handle_input(InputAction action, World* world = nullptr);

private:
    Inventory* _inventory = nullptr;
    Vec3 _position{};
    bool _inventory_open = false;
    int _block_index = 0;
};

} // namespace Balmung

