#pragma once

#include "ecs/components.hpp"
#include "ecs/registry.hpp"
#include "ecs/systems.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Balmung {

class EntityManager {
public:
    ECS::Entity create_entity(const std::string& name = {});
    bool destroy_entity(ECS::Entity entity);
    [[nodiscard]] bool is_alive(ECS::Entity entity) const noexcept { return _registry.is_alive(entity); }
    [[nodiscard]] std::size_t alive_count() const noexcept { return _registry.alive_count(); }

    ECS::Registry& registry() noexcept { return _registry; }
    const ECS::Registry& registry() const noexcept { return _registry; }

    void add_system(std::unique_ptr<ECS::ISystem> system);
    void update(float dt);
    void clear();

private:
    ECS::Registry _registry;
    std::vector<std::unique_ptr<ECS::ISystem>> _systems;
};

} // namespace Balmung

