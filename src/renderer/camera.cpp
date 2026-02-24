#include "renderer/camera.hpp"
#include "renderer/shader.hpp"
#include <cmath>
#include <algorithm>

namespace VoxelBlock {

Vec3 Camera::normalize(Vec3 v) {
    float l = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    if (l < 1e-6f) return {0,0,0};
    return {v.x/l, v.y/l, v.z/l};
}

Vec3 Camera::cross(Vec3 a, Vec3 b) {
    return {a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x};
}

void Camera::update_vectors() {
    float yr = to_rad(yaw);
    float pr = to_rad(pitch);
    front = normalize({
        std::cos(yr) * std::cos(pr),
        std::sin(pr),
        std::sin(yr) * std::cos(pr)
    });
    right = normalize(cross(front, {0,1,0}));
    up    = normalize(cross(right, front));
}

void Camera::process_mouse(float dx, float dy) {
    yaw   += dx * sensitivity;
    pitch -= dy * sensitivity;
    pitch = std::clamp(pitch, -89.f, 89.f);
    update_vectors();
}

void Camera::move(Vec3 dir, float dt) {
    position.x += dir.x * move_speed * dt;
    position.y += dir.y * move_speed * dt;
    position.z += dir.z * move_speed * dt;
}

Mat4 Camera::view_matrix() const {
    Vec3 center = {position.x + front.x,
                   position.y + front.y,
                   position.z + front.z};
    return look_at(position, center, up);
}

Mat4 Camera::projection_matrix(float aspect) const {
    return perspective(to_rad(fov), aspect, near_plane, far_plane);
}

} // namespace VoxelBlock
