#pragma once

#include <glm/glm.hpp>

#include <string>
#include <algorithm>
#include <cmath>

class Terrain;

struct TankConfig {
    float max_health = 100;
    float speed = 5.0f;
    float turn_speed = 90.0f;
    float barrel_length = 1.5f;
    float barrel_angle_min = -10.0f;
    float barrel_angle_max = 80.0f;
};

class Tank {
public:
    Tank(glm::vec3 pos, glm::vec4 color, const TankConfig& cfg)
        : pos(pos), color(color), cfg_(cfg), health(cfg.max_health) {}

    void update(float dt, const Terrain& terrain) {
        if (!is_alive()) return;
        update_physics(dt, pos.y = terrain.get_height(pos.x, pos.z));
    }

    void take_damage(float amount) {
        health -= amount;
        health = std::max(health, 0.0f);
    }

    bool is_alive() const { return health > 0.0f; }

    glm::vec3 position() const { return pos; }

    glm::vec3 get_muzzle_position() const {
        float heading_rad = glm::radians(heading);
        float barrel_rad = glm::radians(barrel_angle);

        glm::vec3 forward(std::sin(heading_rad), 0.0f, std::cos(heading_rad));
        glm::vec3 up(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(up, forward));

        glm::vec3 barrel_dir = glm::normalize(
            forward * std::cos(barrel_rad) + up * std::sin(barrel_rad)
        );

        return pos + glm::vec3(0.0f, 0.8f, 0.0f) + barrel_dir * cfg_.barrel_length;
    }

    glm::vec3 get_fire_direction() const {
        float heading_rad = glm::radians(heading);
        float barrel_rad = glm::radians(barrel_angle);

        glm::vec3 forward(std::sin(heading_rad), 0.0f, std::cos(heading_rad));
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        return glm::normalize(
            forward * std::cos(barrel_rad) + up * std::sin(barrel_rad)
        );
    }

    void aim_towards(glm::vec3 target, const Terrain& terrain) {
        glm::vec3 to_target = target - pos;
        heading = glm::degrees(std::atan2(to_target.x, to_target.z));

        float horizontal_dist = glm::length(glm::vec2(to_target.x, to_target.z));
        float vertical_dist = to_target.y;
        barrel_angle = glm::degrees(std::atan2(vertical_dist, horizontal_dist));
        barrel_angle = std::clamp(barrel_angle, cfg_.barrel_angle_min, cfg_.barrel_angle_max);
    }

    float move_input = 0;
    float turn_input = 0;
    float barrel_angle = 30.0f;
    float power = 50.0f;
    bool wants_to_fire = false;

    float health;
    glm::vec3 pos;
    float heading = 0;
    glm::vec4 color;
    bool is_player = false;
    std::string name;

private:
    TankConfig cfg_;

    void update_physics(float dt, float terrain_y) {
        heading += turn_input * cfg_.turn_speed * dt;

        float heading_rad = glm::radians(heading);
        glm::vec3 forward(std::sin(heading_rad), 0.0f, std::cos(heading_rad));

        pos += forward * move_input * cfg_.speed * dt;
        pos.y = terrain_y;
    }
};
