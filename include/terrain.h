#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

class Terrain {
public:
    Terrain(int width, int height, float scale, float height_scale)
        : width_(width), height_(height), scale_(scale), height_scale_(height_scale),
          heightmap_(width * height, 0.0f) {}

    void generate() {
        for (int z = 0; z < height_; ++z) {
            for (int x = 0; x < width_; ++x) {
                float nx = static_cast<float>(x) / width_;
                float nz = static_cast<float>(z) / height_;

                float h = 0.0f;

                h += std::sin(nx * 6.2831f * 1.0f + nz * 3.1415f) * 0.5f;
                h += std::cos(nz * 6.2831f * 2.0f + nx * 1.5707f) * 0.25f;
                h += std::sin((nx + nz) * 6.2831f * 3.0f) * 0.125f;
                h += std::cos((nx - nz) * 6.2831f * 4.0f) * 0.0625f;
                h += std::sin(nx * 12.566f) * std::cos(nz * 12.566f) * 0.0625f;

                h *= height_scale_;
                heightmap_[z * width_ + x] = h;
            }
        }
    }

    void destroy_at(glm::vec3 world_pos, float radius) {
        float cx = (world_pos.x / scale_) + width_ * 0.5f;
        float cz = (world_pos.z / scale_) + height_ * 0.5f;
        float r = radius / scale_;

        int ix_min = std::max(0, static_cast<int>(std::floor(cx - r)));
        int ix_max = std::min(width_ - 1, static_cast<int>(std::ceil(cx + r)));
        int iz_min = std::max(0, static_cast<int>(std::floor(cz - r)));
        int iz_max = std::min(height_ - 1, static_cast<int>(std::ceil(cz + r)));

        for (int z = iz_min; z <= iz_max; ++z) {
            for (int x = ix_min; x <= ix_max; ++x) {
                float dx = x - cx;
                float dz = z - cz;
                float dist = std::sqrt(dx * dx + dz * dz);

                if (dist < r) {
                    float falloff = 1.0f - (dist / r);
                    falloff = falloff * falloff;
                    float depth = world_pos.y + height_scale_ * 0.5f;
                    float crater_depth = depth * falloff;
                    heightmap_[z * width_ + x] -= crater_depth;
                }
            }
        }
        rebuild_mesh();
    }

    float get_height(float x, float z) const {
        float gx = (x / scale_) + width_ * 0.5f;
        float gz = (z / scale_) + height_ * 0.5f;

        int ix = static_cast<int>(std::floor(gx));
        int iz = static_cast<int>(std::floor(gz));

        ix = std::clamp(ix, 0, width_ - 2);
        iz = std::clamp(iz, 0, height_ - 2);

        float tx = gx - ix;
        float tz = gz - iz;

        float h00 = heightmap_[iz * width_ + ix];
        float h10 = heightmap_[iz * width_ + ix + 1];
        float h01 = heightmap_[(iz + 1) * width_ + ix];
        float h11 = heightmap_[(iz + 1) * width_ + ix + 1];

        float h0 = h00 + (h10 - h00) * tx;
        float h1 = h01 + (h11 - h01) * tx;

        return h0 + (h1 - h0) * tz;
    }

    glm::vec3 get_normal(float x, float z) const {
        float eps = scale_ * 0.5f;
        float hL = get_height(x - eps, z);
        float hR = get_height(x + eps, z);
        float hD = get_height(x, z - eps);
        float hU = get_height(x, z + eps);

        glm::vec3 normal(hL - hR, 2.0f * eps, hD - hU);
        return glm::normalize(normal);
    }

    void upload_gpu() {
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);
        rebuild_mesh();
    }

    void render() {
        glBindVertexArray(vao_);
        glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    int width() const { return width_; }
    int height() const { return height_; }

    float world_x(float grid_x) const {
        return (grid_x - width_ * 0.5f) * scale_;
    }

    float world_z(float grid_z) const {
        return (grid_z - height_ * 0.5f) * scale_;
    }

    float scale() const { return scale_; }
    const std::vector<float>& heightmap() const { return heightmap_; }

private:
    int width_, height_;
    float scale_, height_scale_;
    std::vector<float> heightmap_;
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    int index_count_ = 0;

    void rebuild_mesh() {
        struct Vertex {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texcoord;
        };

        std::vector<Vertex> vertices(width_ * height_);
        std::vector<unsigned int> indices;

        for (int z = 0; z < height_; ++z) {
            for (int x = 0; x < width_; ++x) {
                float wx = world_x(static_cast<float>(x));
                float wz = world_z(static_cast<float>(z));
                float h = heightmap_[z * width_ + x];

                glm::vec3 pos(wx, h, wz);

                float eps = scale_ * 0.5f;
                float hL = (x > 0) ? heightmap_[z * width_ + (x - 1)] : h;
                float hR = (x < width_ - 1) ? heightmap_[z * width_ + (x + 1)] : h;
                float hD = (z > 0) ? heightmap_[(z - 1) * width_ + x] : h;
                float hU = (z < height_ - 1) ? heightmap_[(z + 1) * width_ + x] : h;

                glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU));

                float u = static_cast<float>(x) / (width_ - 1);
                float v = static_cast<float>(z) / (height_ - 1);

                vertices[z * width_ + x] = { pos, normal, glm::vec2(u, v) };
            }
        }

        for (int z = 0; z < height_ - 1; ++z) {
            for (int x = 0; x < width_ - 1; ++x) {
                unsigned int tl = z * width_ + x;
                unsigned int tr = tl + 1;
                unsigned int bl = (z + 1) * width_ + x;
                unsigned int br = bl + 1;

                indices.push_back(tl);
                indices.push_back(bl);
                indices.push_back(tr);

                indices.push_back(tr);
                indices.push_back(bl);
                indices.push_back(br);
            }
        }

        index_count_ = static_cast<int>(indices.size());

        glBindVertexArray(vao_);

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

        glBindVertexArray(0);
    }
};
