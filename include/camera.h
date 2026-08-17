#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

class Camera {
public:
    Camera(glm::vec3 target, float distance, float pitch, float yaw)
        : target_(target), distance_(distance),
          pitch_(pitch), yaw_(yaw),
          smooth_target_(target), smooth_distance_(distance),
          smooth_pitch_(pitch), smooth_yaw_(yaw) {}

    glm::mat4 view_matrix() const {
        return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 projection_matrix(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, near, far);
    }

    void orbit(float dx, float dy) {
        yaw_ += dx;
        pitch_ += dy;
        pitch_ = std::clamp(pitch_, pitch_min, pitch_max);
    }

    void zoom(float delta) {
        distance_ -= delta;
        distance_ = std::clamp(distance_, distance_min, distance_max);
    }

    void follow(glm::vec3 target, float dt) {
        float smoothing = 1.0f - std::exp(-5.0f * dt);
        target_ = glm::mix(target_, target, smoothing);
        smooth_target_ = glm::mix(smooth_target_, target_, smoothing);
        smooth_distance_ = glm::mix(smooth_distance_, distance_, smoothing);
        smooth_pitch_ = glm::mix(smooth_pitch_, pitch_, smoothing);
        smooth_yaw_ = glm::mix(smooth_yaw_, yaw_, smoothing);
    }

    void add_shake(float amount) { shake_amount_ = std::max(shake_amount_, amount); }

    glm::vec3 position() const {
        float pitch_rad = glm::radians(smooth_pitch_);
        float yaw_rad = glm::radians(smooth_yaw_);
        float shake_x = 0, shake_y = 0;
        if (shake_amount_ > 0.01f) {
            shake_x = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * shake_amount_;
            shake_y = (static_cast<float>(std::rand()) / RAND_MAX - 0.5f) * shake_amount_;
        }
        return smooth_target_ + glm::vec3(
            smooth_distance_ * std::cos(pitch_rad) * std::sin(yaw_rad) + shake_x,
            smooth_distance_ * std::sin(pitch_rad) + shake_y,
            smooth_distance_ * std::cos(pitch_rad) * std::cos(yaw_rad)
        );
    }

    glm::vec3 front() const {
        return glm::normalize(target_ - position());
    }

    glm::vec3 right() const {
        return glm::normalize(glm::cross(front(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    glm::vec3 up() const {
        return glm::normalize(glm::cross(right(), front()));
    }

    float fov = 60.0f;
    float near = 0.1f;
    float far = 500.0f;

    void update_shake(float dt) {
        if (shake_amount_ > 0.01f) shake_amount_ *= std::exp(-8.0f * dt);
        else shake_amount_ = 0;
    }

private:
    glm::vec3 target_, smooth_target_;
    float distance_, pitch_, yaw_;
    float smooth_distance_, smooth_pitch_, smooth_yaw_;
    float shake_amount_ = 0;
    float distance_min = 5.0f, distance_max = 100.0f;
    float pitch_min = -89.0f, pitch_max = -5.0f;
};
