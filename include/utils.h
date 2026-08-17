#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

inline std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

inline GLuint compile_shader(const std::string& source, GLenum type) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compilation failed:\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

inline GLuint create_program(const std::string& vert_path, const std::string& frag_path) {
    std::string vert_src = read_file(vert_path);
    std::string frag_src = read_file(frag_path);

    if (vert_src.empty() || frag_src.empty()) return 0;

    GLuint vert = compile_shader(vert_src, GL_VERTEX_SHADER);
    GLuint frag = compile_shader(frag_src, GL_FRAGMENT_SHADER);
    if (!vert || !frag) return 0;

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Program linking failed:\n" << log << "\n";
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

inline float height_at(const std::vector<float>& heightmap, int width, int height, float x, float z) {
    float fx = x * (width - 1);
    float fz = z * (height - 1);

    int ix = static_cast<int>(std::floor(fx));
    int iz = static_cast<int>(std::floor(fz));

    ix = std::clamp(ix, 0, width - 2);
    iz = std::clamp(iz, 0, height - 2);

    float tx = fx - ix;
    float tz = fz - iz;

    float h00 = heightmap[iz * width + ix];
    float h10 = heightmap[iz * width + ix + 1];
    float h01 = heightmap[(iz + 1) * width + ix];
    float h11 = heightmap[(iz + 1) * width + ix + 1];

    float h0 = h00 + (h10 - h00) * tx;
    float h1 = h01 + (h11 - h01) * tx;

    return h0 + (h1 - h0) * tz;
}

inline float rand_float(float min, float max) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

inline glm::vec3 rand_vec3(float min, float max) {
    return glm::vec3(
        rand_float(min, max),
        rand_float(min, max),
        rand_float(min, max)
    );
}
