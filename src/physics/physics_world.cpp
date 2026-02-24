#include "physics/physics_world.hpp"

namespace VoxelBlock::Physics {

PhysicsWorld::PhysicsWorld()
    : _backend(make_null_physics_backend())
{
}

PhysicsWorld::PhysicsWorld(std::unique_ptr<IPhysicsBackend> backend)
    : _backend(backend ? std::move(backend) : make_null_physics_backend())
{
}

PhysicsWorld::~PhysicsWorld() = default;

void PhysicsWorld::set_backend(std::unique_ptr<IPhysicsBackend> backend)
{
    _backend = backend ? std::move(backend) : make_null_physics_backend();
}

std::uint32_t PhysicsWorld::create_body(const RigidBodyDesc& desc, const Transform& initial_transform)
{
    return _backend->create_body(desc, initial_transform);
}

bool PhysicsWorld::destroy_body(std::uint32_t body_id)
{
    return _backend->destroy_body(body_id);
}

bool PhysicsWorld::set_body_transform(std::uint32_t body_id, const Transform& transform)
{
    return _backend->set_body_transform(body_id, transform);
}

std::optional<BodyState> PhysicsWorld::get_body_state(std::uint32_t body_id) const
{
    return _backend->get_body_state(body_id);
}

void PhysicsWorld::step(float dt)
{
    if (dt <= 0.0f) return;
    _backend->step(dt);
}

void PhysicsWorld::reset()
{
    _backend->reset();
}

} // namespace VoxelBlock::Physics

