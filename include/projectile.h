#pragma once

#include <glm/glm.hpp>

#include <string>
#include <cmath>

enum class WeaponType { Cannon, Missile, Nuke, Laser };

struct Weapon {
    std::string name = "Cannon";
    float speed = 30.0f;
    float gravity = 9.81f;
    float damage = 25.0f;
    float explosion_radius = 2.0f;
    WeaponType type = WeaponType::Cannon;
};

class Projectile {
public:
    Projectile() = default;

    Projectile(glm::vec3 pos, glm::vec3 vel, const Weapon& weapon)
        : pos_(pos), vel_(vel), weapon(weapon), in_flight(true), has_hit(false) {}

    bool update(float dt, const Terrain& terrain, float terrain_width, float terrain_height, float terrain_scale) {
        if (!in_flight) return has_hit;

        lifetime_ += dt;

        if (weapon.type == WeaponType::Laser) {
            pos_ += vel_ * dt;
        } else {
            vel_.y -= weapon.gravity * dt;
            pos_ += vel_ * dt;
        }

        float half_w = terrain_width * terrain_scale * 0.5f;
        float half_h = terrain_height * terrain_scale * 0.5f;

        bool out_of_bounds = pos_.x < -half_w || pos_.x > half_w ||
                             pos_.z < -half_h || pos_.z > half_h;

        float terrain_y = terrain.get_height(pos_.x, pos_.z);
        bool hit_ground = pos_.y <= terrain_y;

        bool timed_out = lifetime_ > 10.0f;

        if (out_of_bounds || hit_ground || timed_out) {
            in_flight = false;
            has_hit = true;
        }

        return has_hit;
    }

    bool has_hit = false;
    bool in_flight = false;

    glm::vec3 position() const { return pos_; }
    glm::vec3 velocity() const { return vel_; }

    Weapon weapon;

private:
    glm::vec3 pos_ = glm::vec3(0.0f);
    glm::vec3 vel_ = glm::vec3(0.0f);
    float lifetime_ = 0.0f;
};
