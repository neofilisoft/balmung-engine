#include "physics/physics_world.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#if __has_include(<AsterCore/AsterCore.h>)
#define BM_ASTERCORE_RUNTIME_AVAILABLE 1
#include <AsterCore/AsterCore.h>
#include <AsterCore/RegisterTypes.h>
#include <AsterCore/Core/Factory.h>
#include <AsterCore/Core/JobSystemThreadPool.h>
#include <AsterCore/Core/TempAllocator.h>
#include <AsterCore/Math/Quat.h>
#include <AsterCore/Physics/Body/BodyCreationSettings.h>
#include <AsterCore/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <AsterCore/Physics/Collision/ObjectLayer.h>
#include <AsterCore/Physics/Collision/Shape/BoxShape.h>
#include <AsterCore/Physics/EActivation.h>
#include <AsterCore/Physics/PhysicsSystem.h>
#endif

namespace Balmung::Physics {

#if BM_ASTERCORE_RUNTIME_AVAILABLE
namespace {

constexpr float kDegToRad = 0.01745329251994329577f;
constexpr float kRadToDeg = 57.295779513082320876f;
constexpr ACPH::ObjectLayer kStaticLayer = 0;
constexpr ACPH::ObjectLayer kMovingLayer = 1;

void AsterTraceBridge(const char* in_format, ...)
{
    (void)in_format;
}

struct AsterCoreBootstrap {
    AsterCoreBootstrap()
    {
        ACPH::RegisterDefaultAllocator();
        ACPH::Trace = AsterTraceBridge;
        if (ACPH::Factory::sInstance == nullptr) {
            ACPH::Factory::sInstance = new ACPH::Factory();
            ACPH::RegisterTypes();
        }
    }

    ~AsterCoreBootstrap()
    {
        if (ACPH::Factory::sInstance != nullptr) {
            ACPH::UnregisterTypes();
            delete ACPH::Factory::sInstance;
            ACPH::Factory::sInstance = nullptr;
        }
    }
};

AsterCoreBootstrap& ensure_astercore_bootstrap()
{
    static AsterCoreBootstrap bootstrap;
    return bootstrap;
}

class BalmungObjectLayerPairFilter final : public ACPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(ACPH::ObjectLayer object1, ACPH::ObjectLayer object2) const override
    {
        if (object1 == kStaticLayer) {
            return object2 == kMovingLayer;
        }
        return true;
    }
};

class BalmungBroadPhaseLayerInterface final : public ACPH::BroadPhaseLayerInterface {
public:
    BalmungBroadPhaseLayerInterface()
    {
        _mapping[kStaticLayer] = ACPH::BroadPhaseLayer(0);
        _mapping[kMovingLayer] = ACPH::BroadPhaseLayer(1);
    }

    ACPH::uint GetNumBroadPhaseLayers() const override
    {
        return 2;
    }

    ACPH::BroadPhaseLayer GetBroadPhaseLayer(ACPH::ObjectLayer layer) const override
    {
        return _mapping[layer == kStaticLayer ? 0 : 1];
    }

#if defined(ACPH_EXTERNAL_PROFILE) || defined(ACPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(ACPH::BroadPhaseLayer layer) const override
    {
        return layer == ACPH::BroadPhaseLayer(0) ? "Static" : "Moving";
    }
#endif

private:
    ACPH::BroadPhaseLayer _mapping[2];
};

class BalmungObjectVsBroadPhaseLayerFilter final : public ACPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(ACPH::ObjectLayer layer, ACPH::BroadPhaseLayer broad_phase_layer) const override
    {
        if (layer == kStaticLayer) {
            return broad_phase_layer == ACPH::BroadPhaseLayer(1);
        }
        return true;
    }
};

ACPH::RVec3 to_aster_position(const Vec3& value)
{
    return ACPH::RVec3(value.x, value.y, value.z);
}

ACPH::Vec3 to_aster_vec3(const Vec3& value)
{
    return ACPH::Vec3(value.x, value.y, value.z);
}

Vec3 from_aster_position(const ACPH::RVec3& value)
{
    return Vec3{static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ())};
}

Vec3 from_aster_vec3(const ACPH::Vec3& value)
{
    return Vec3{value.GetX(), value.GetY(), value.GetZ()};
}

ACPH::Quat to_aster_rotation(const Vec3& rotation_euler_deg)
{
    return ACPH::Quat::sEulerAngles(
        ACPH::Vec3(
            rotation_euler_deg.x * kDegToRad,
            rotation_euler_deg.y * kDegToRad,
            rotation_euler_deg.z * kDegToRad));
}

Vec3 from_aster_rotation(const ACPH::Quat& rotation)
{
    const ACPH::Vec3 radians = rotation.GetEulerAngles();
    return Vec3{
        radians.GetX() * kRadToDeg,
        radians.GetY() * kRadToDeg,
        radians.GetZ() * kRadToDeg};
}

ACPH::EMotionType to_aster_motion_type(BodyType type)
{
    switch (type) {
    case BodyType::Dynamic: return ACPH::EMotionType::Dynamic;
    case BodyType::Kinematic: return ACPH::EMotionType::Kinematic;
    case BodyType::Static:
    default: return ACPH::EMotionType::Static;
    }
}

ACPH::ObjectLayer to_aster_object_layer(BodyType type)
{
    return type == BodyType::Static ? kStaticLayer : kMovingLayer;
}

class AsterCorePhysicsBackend final : public IPhysicsBackend {
public:
    AsterCorePhysicsBackend()
        : _bootstrap(ensure_astercore_bootstrap())
        , _temp_allocator(std::make_unique<ACPH::TempAllocatorImpl>(8 * 1024 * 1024))
        , _job_system(std::make_unique<ACPH::JobSystemThreadPool>(
              ACPH::cMaxPhysicsJobs,
              ACPH::cMaxPhysicsBarriers,
              std::max(1u, std::thread::hardware_concurrency() > 1 ? std::thread::hardware_concurrency() - 1 : 1u)))
    {
        _physics_system.Init(
            8192,
            0,
            8192,
            4096,
            _broad_phase_layers,
            _object_vs_broadphase_filter,
            _object_layer_pair_filter);
    }

    const char* backend_name() const noexcept override { return "astercore"; }

    void reset() override
    {
        for (const auto& [id, body_id] : _body_ids) {
            (void)id;
            if (!body_id.IsInvalid()) {
                _body_interface().RemoveBody(body_id);
                _body_interface().DestroyBody(body_id);
            }
        }
        _body_ids.clear();
        _next_id = 1;
    }

    std::uint32_t create_body(const RigidBodyDesc& desc, const Transform& initial_transform) override
    {
        ACPH::BoxShapeSettings shape_settings(to_aster_vec3(desc.box_half_extents));
        shape_settings.SetEmbedded();
        ACPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();
        if (shape_result.HasError()) {
            return 0;
        }

        ACPH::BodyCreationSettings settings(
            shape_result.Get(),
            to_aster_position(initial_transform.position),
            to_aster_rotation(initial_transform.rotation_euler_deg),
            to_aster_motion_type(desc.type),
            to_aster_object_layer(desc.type));
        settings.mGravityFactor = desc.use_gravity ? 1.0f : 0.0f;
        settings.mAllowSleeping = (desc.type == BodyType::Dynamic);
        settings.mOverrideMassProperties = ACPH::EOverrideMassProperties::CalculateInertia;
        settings.mMassPropertiesOverride.mMass = std::max(desc.mass, 0.001f);

        const ACPH::EActivation activation = desc.type == BodyType::Static
            ? ACPH::EActivation::DontActivate
            : ACPH::EActivation::Activate;
        ACPH::BodyID body_id = _body_interface().CreateAndAddBody(settings, activation);
        if (body_id.IsInvalid()) {
            return 0;
        }

        const auto id = _next_id++;
        _body_ids[id] = body_id;
        return id;
    }

    bool destroy_body(std::uint32_t body_id) override
    {
        const auto it = _body_ids.find(body_id);
        if (it == _body_ids.end()) {
            return false;
        }

        if (!it->second.IsInvalid()) {
            _body_interface().RemoveBody(it->second);
            _body_interface().DestroyBody(it->second);
        }
        _body_ids.erase(it);
        return true;
    }

    bool set_body_transform(std::uint32_t body_id, const Transform& transform) override
    {
        const auto it = _body_ids.find(body_id);
        if (it == _body_ids.end() || it->second.IsInvalid()) {
            return false;
        }

        _body_interface().SetPositionAndRotation(
            it->second,
            to_aster_position(transform.position),
            to_aster_rotation(transform.rotation_euler_deg),
            ACPH::EActivation::Activate);
        return true;
    }

    std::optional<BodyState> get_body_state(std::uint32_t body_id) const override
    {
        const auto it = _body_ids.find(body_id);
        if (it == _body_ids.end() || it->second.IsInvalid()) {
            return std::nullopt;
        }

        BodyState state{};
        state.id = body_id;
        state.transform.position = from_aster_position(_body_interface().GetPosition(it->second));
        state.transform.rotation_euler_deg = from_aster_rotation(_body_interface().GetRotation(it->second));
        state.linear_velocity = from_aster_vec3(_body_interface().GetLinearVelocity(it->second));
        state.sleeping = !_body_interface().IsActive(it->second);
        return state;
    }

    void step(float dt) override
    {
        if (dt <= 0.0f) {
            return;
        }

        const int collision_steps = std::max(1, static_cast<int>(std::ceil(dt * 60.0f)));
        _physics_system.Update(dt, collision_steps, _temp_allocator.get(), _job_system.get());
    }

private:
    ACPH::BodyInterface& _body_interface() { return _physics_system.GetBodyInterface(); }
    const ACPH::BodyInterface& _body_interface() const { return _physics_system.GetBodyInterface(); }

    AsterCoreBootstrap& _bootstrap;
    BalmungBroadPhaseLayerInterface _broad_phase_layers;
    BalmungObjectVsBroadPhaseLayerFilter _object_vs_broadphase_filter;
    BalmungObjectLayerPairFilter _object_layer_pair_filter;
    ACPH::PhysicsSystem _physics_system;
    std::unique_ptr<ACPH::TempAllocatorImpl> _temp_allocator;
    std::unique_ptr<ACPH::JobSystemThreadPool> _job_system;
    std::uint32_t _next_id = 1;
    std::unordered_map<std::uint32_t, ACPH::BodyID> _body_ids;
};

} // namespace
#endif

std::unique_ptr<IPhysicsBackend> make_astercore_physics_backend()
{
#if BM_ASTERCORE_RUNTIME_AVAILABLE
    return std::make_unique<AsterCorePhysicsBackend>();
#else
    return make_null_physics_backend();
#endif
}

} // namespace Balmung::Physics
