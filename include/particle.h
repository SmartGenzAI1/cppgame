#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float life;
    float max_life;
    float size;
    glm::vec4 color;
};

class ParticleSystem {
public:
    void emit_explosion(glm::vec3 pos, glm::vec3 color, int count = 50) {
        for (int i = 0; i < count; ++i) {
            float rx = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
            float ry = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
            float rz = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;

            float speed = 2.0f + static_cast<float>(std::rand()) / RAND_MAX * 10.0f;
            float life = 0.4f + static_cast<float>(std::rand()) / RAND_MAX * 1.2f;

            Particle p;
            p.position = pos;
            p.velocity = glm::normalize(glm::vec3(rx, std::abs(ry) + 0.3f, rz)) * speed;
            p.life = life;
            p.max_life = life;
            p.size = 0.3f + static_cast<float>(std::rand()) / RAND_MAX * 0.8f;

            float r = color.r + (static_cast<float>(std::rand()) / RAND_MAX) * 0.3f;
            float g = color.g * (0.5f + static_cast<float>(std::rand()) / RAND_MAX * 0.5f);
            p.color = glm::vec4(r, g, color.b * 0.3f, 1.0f);
            particles_.push_back(p);
        }
    }

    void emit_smoke(glm::vec3 pos, int count = 20) {
        for (int i = 0; i < count; ++i) {
            float rx = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
            float rz = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
            float life = 1.0f + static_cast<float>(std::rand()) / RAND_MAX * 3.0f;

            Particle p;
            p.position = pos + glm::vec3(rx * 0.5f, 0.0f, rz * 0.5f);
            p.velocity = glm::vec3(rx * 0.3f, 1.5f + static_cast<float>(std::rand()) / RAND_MAX * 2.0f, rz * 0.3f);
            p.life = life;
            p.max_life = life;
            p.size = 0.5f + static_cast<float>(std::rand()) / RAND_MAX * 1.0f;
            p.color = glm::vec4(0.4f, 0.4f, 0.4f, 0.5f);
            particles_.push_back(p);
        }
    }

    void emit_trail(glm::vec3 pos, glm::vec3 vel) {
        Particle p;
        p.position = pos;
        p.velocity = -vel * 0.03f + glm::vec3(
            (static_cast<float>(std::rand()) / RAND_MAX) * 0.3f - 0.15f,
            (static_cast<float>(std::rand()) / RAND_MAX) * 0.3f - 0.15f,
            (static_cast<float>(std::rand()) / RAND_MAX) * 0.3f - 0.15f
        );
        p.life = 0.2f + static_cast<float>(std::rand()) / RAND_MAX * 0.4f;
        p.max_life = p.life;
        p.size = 0.08f + static_cast<float>(std::rand()) / RAND_MAX * 0.1f;
        p.color = glm::vec4(1.0f, 0.7f, 0.1f, 1.0f);
        particles_.push_back(p);
    }

    void update(float dt) {
        for (auto& p : particles_) {
            p.position += p.velocity * dt;
            p.velocity.y -= 4.0f * dt;
            p.velocity *= 0.99f;
            p.life -= dt;
        }
        particles_.erase(
            std::remove_if(particles_.begin(), particles_.end(),
                [](const Particle& p) { return p.life <= 0.0f; }),
            particles_.end()
        );
    }

    void upload_gpu() {
        float quad[] = {
            -0.5f, -0.5f, 0.0f,
             0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f, -0.5f, 0.0f,
             0.5f,  0.5f, 0.0f,
            -0.5f,  0.5f, 0.0f,
        };
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    void render(const glm::mat4& view, const glm::mat4& proj,
                const glm::vec3& camRight, const glm::vec3& camUp,
                GLuint shader) {
        if (particles_.empty()) return;
        glUseProgram(shader);
        glBindVertexArray(vao_);
        for (const auto& p : particles_) {
            float alpha = std::clamp(p.life / p.max_life, 0.0f, 1.0f);
            glm::vec4 col = p.color;
            col.a *= alpha;
            float size = p.size * (0.5f + 0.5f * (p.life / p.max_life));

            glUniform3fv(glGetUniformLocation(shader, "particleCenter"), 1, &p.position[0]);
            glUniform1f(glGetUniformLocation(shader, "particleSize"), size);
            glUniform4fv(glGetUniformLocation(shader, "particleColor"), 1, &col[0]);
            glUniform3fv(glGetUniformLocation(shader, "cameraRight"), 1, &camRight[0]);
            glUniform3fv(glGetUniformLocation(shader, "cameraUp"), 1, &camUp[0]);

            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);
    }

    const std::vector<Particle>& particles() const { return particles_; }

private:
    std::vector<Particle> particles_;
    GLuint vao_ = 0, vbo_ = 0;
};
