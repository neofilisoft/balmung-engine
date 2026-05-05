#include "core/entity_manager.hpp"

namespace Balmung {

ECS::Entity EntityManager::create_entity(const std::string& name)
{
    auto entity = _registry.create();
    if (!name.empty()) {
        _registry.emplace_or_replace<ECS::NameComponent>(entity, name);
    }
    _registry.emplace_or_replace<ECS::TransformComponent>(entity);
    return entity;
}

bool EntityManager::destroy_entity(ECS::Entity entity)
{
    return _registry.destroy(entity);
}

void EntityManager::add_system(std::unique_ptr<ECS::ISystem> system)
{
    if (system) {
        _systems.push_back(std::move(system));
    }
}

void EntityManager::update(float dt)
{
    for (auto& system : _systems) {
        system->update(_registry, dt);
    }
}

void EntityManager::clear()
{
    _systems.clear();
    _registry.clear();
}

} // namespace Balmung

