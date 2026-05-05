#include "core/player.hpp"

#include "core/world.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace Balmung {

Player::Player(Inventory* inventory)
    : _inventory(inventory)
{
}

void Player::set_block_index(int index) noexcept
{
    _block_index = std::clamp(index, 0, 8);
}

void Player::handle_input(InputAction action, World* world)
{
    static constexpr std::array<const char*, 9> hotbar{
        "grass", "dirt", "stone", "wood", "leaves", "sand", "planks", "cobble", "water"
    };

    switch (action) {
    case InputAction::ScrollUp:
        _block_index = (_block_index + 8) % 9;
        break;
    case InputAction::ScrollDown:
        _block_index = (_block_index + 1) % 9;
        break;
    case InputAction::PlaceBlock:
        if (world) {
            world->place_voxel(static_cast<int>(std::round(_position.x)),
                               static_cast<int>(std::round(_position.y - 2.0f)),
                               static_cast<int>(std::round(_position.z - 3.0f)),
                               hotbar[static_cast<std::size_t>(_block_index)]);
        }
        break;
    case InputAction::BreakBlock:
        if (world) {
            world->destroy_voxel(static_cast<int>(std::round(_position.x)),
                                 static_cast<int>(std::round(_position.y - 2.0f)),
                                 static_cast<int>(std::round(_position.z - 3.0f)));
        }
        break;
    }
}

} // namespace Balmung

