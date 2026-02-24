#include "physics/physics_world.hpp"

#include <unordered_map>

namespace VoxelBlock::Physics {
namespace {

class NullPhysicsBackend final : public IPhysicsBackend {
public:
    const char* backend_name() const noexcept override { return "none"; }

    void reset() override
    {
        _states.clear();
        _next_id = 1;
    }

    std::uint32_t create_body(const RigidBodyDesc& desc, const Transform& initial_transform) override
    {
        const auto id = _next_id++;
        BodyState state{};
        state.id = id;
        state.transform = initial_transform;
        state.sleeping = (desc.type == BodyType::Static);
        _descs[id] = desc;
        _states[id] = state;
        return id;
    }

    bool destroy_body(std::uint32_t body_id) override
    {
        const auto erased_a = _descs.erase(body_id);
        const auto erased_b = _states.erase(body_id);
        return (erased_a + erased_b) > 0;
    }

    bool set_body_transform(std::uint32_t body_id, const Transform& transform) override
    {
        auto it = _states.find(body_id);
        if (it == _states.end()) return false;
        it->second.transform = transform;
        return true;
    }

    std::optional<BodyState> get_body_state(std::uint32_t body_id) const override
    {
        auto it = _states.find(body_id);
        if (it == _states.end()) return std::nullopt;
        return it->second;
    }

    void step(float dt) override
    {
        if (dt <= 0.0f) return;
        for (auto& [id, state] : _states) {
            const auto desc_it = _descs.find(id);
            if (desc_it == _descs.end()) continue;
            const auto& desc = desc_it->second;
            if (desc.type != BodyType::Dynamic || !desc.use_gravity) continue;

            state.linear_velocity.y -= 9.81f * dt;
            state.transform.position.y += state.linear_velocity.y * dt;
            state.sleeping = false;
        }
    }

private:
    std::uint32_t _next_id = 1;
    std::unordered_map<std::uint32_t, RigidBodyDesc> _descs;
    std::unordered_map<std::uint32_t, BodyState> _states;
};

} // namespace

std::unique_ptr<IPhysicsBackend> make_null_physics_backend()
{
    return std::make_unique<NullPhysicsBackend>();
}

} // namespace VoxelBlock::Physics

