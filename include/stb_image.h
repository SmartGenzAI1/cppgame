#pragma once

// Minimal texture loader for TankWar3D.
// Instead of the full stb_image library, we use GLEW's built-in image loading
// or simple hardcoded color textures. If you need PNG/JPG loading later,
// consider adding stb_image.c to your src/ directory and defining
// STB_IMAGE_IMPLEMENTATION there.

#include <GL/glew.h>
#include <vector>
#include <cstdint>

inline GLuint create_checker_texture(int size = 64, int check_size = 8) {
    std::vector<uint8_t> data(size * size * 3);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            bool white = ((x / check_size) + (y / check_size)) % 2 == 0;
            uint8_t c = white ? 200 : 100;
            int idx = (y * size + x) * 3;
            data[idx + 0] = c;
            data[idx + 1] = c;
            data[idx + 2] = c;
        }
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return tex;
}

inline GLuint create_solid_texture(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t data[3] = { r, g, b };

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return tex;
}
