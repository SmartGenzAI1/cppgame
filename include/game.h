#pragma once

#include "utils.h"
#include "camera.h"
#include "terrain.h"
#include "tank.h"
#include "projectile.h"
#include "particle.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>

enum class GameState { Menu, Playing, Watching, GameOver };

class Game {
public:
    Game(int width, int height);
    bool init();
    void run();
    void shutdown();

private:
    void process_input(float dt);
    void update(float dt);
    void render();
    void render_3d_scene();
    void render_hud();
    void render_menu();
    void render_game_over();
    void fire_projectile();
    void check_explosion();
    void next_turn();

    void init_hud_geometry();
    void draw_hud_rect(float x, float y, float w, float h, glm::vec4 color);
    void draw_hud_rect_gradient(float x, float y, float w, float h, glm::vec4 color, float fill_pct);

    void init_water();
    void draw_water();

    void init_tank_geometry();
    void draw_tank_body();
    void draw_tank_turret();
    void draw_tank_barrel();
    void draw_tank_tracks();

    GLFWwindow* window_ = nullptr;
    int width_, height_;

    GLuint terrain_shader_ = 0;
    GLuint model_shader_ = 0;
    GLuint particle_shader_ = 0;
    GLuint hud_shader_ = 0;
    GLuint water_shader_ = 0;

    GLuint hud_vao_ = 0, hud_vbo_ = 0;
    GLuint water_vao_ = 0, water_vbo_ = 0;
    GLuint tank_vao_ = 0, tank_vbo_ = 0;
    int tank_cube_verts_ = 0;

    Camera camera_;
    Terrain terrain_;
    std::vector<Tank> tanks_;
    Projectile projectile_;
    ParticleSystem particles_;
    int current_tank_ = 0;
    GameState state_ = GameState::Menu;

    std::vector<Weapon> weapons_;
    int current_weapon_ = 0;

    glm::vec2 wind_ = glm::vec2(0.0f);
    float game_time_ = 0.0f;
    float turn_timer_ = 0.0f;
    float ai_delay_ = 0.0f;
    float flash_timer_ = 0.0f;
    float title_blink_ = 0.0f;
};
