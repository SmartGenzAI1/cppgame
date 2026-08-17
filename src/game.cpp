#include "game.h"
#include "utils.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

static Camera* g_camera = nullptr;
static bool g_mouse_down = false;
static double g_last_mx = 0, g_last_my = 0;

static void fb_size_cb(GLFWwindow* w, int width, int height) {
    if (height > 0) glViewport(0, 0, width, height);
}
static void scroll_cb(GLFWwindow* w, double, double yoff) {
    if (g_camera) g_camera->zoom((float)yoff * 2.0f);
}
static void mouse_btn_cb(GLFWwindow* w, int btn, int action, int) {
    if (btn == GLFW_MOUSE_BUTTON_RIGHT) {
        g_mouse_down = (action == GLFW_PRESS);
        if (g_mouse_down) glfwGetCursorPos(w, &g_last_mx, &g_last_my);
    }
}
static void mouse_move_cb(GLFWwindow* w, double mx, double my) {
    if (g_mouse_down && g_camera) {
        g_camera->orbit((float)(g_last_mx - mx), (float)(my - g_last_my));
        g_last_mx = mx; g_last_my = my;
    }
}

static void draw_cube_at(GLuint vao, GLuint shader, glm::vec3 pos, glm::vec3 scale,
                         glm::vec3 color, float emissive = 0.0f) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    model = glm::scale(model, scale);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shader, "objectColor"), color.r, color.g, color.b);
    glUniform1f(glGetUniformLocation(shader, "emissive"), emissive);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

static void draw_cube_rotated(GLuint vao, GLuint shader, glm::vec3 pos, float heading_deg,
                              float pitch_deg, glm::vec3 pivot, glm::vec3 scale,
                              glm::vec3 color) {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, pos);
    model = glm::rotate(model, glm::radians(heading_deg), glm::vec3(0, 1, 0));
    model = glm::translate(model, pivot);
    model = glm::rotate(model, glm::radians(pitch_deg), glm::vec3(1, 0, 0));
    model = glm::scale(model, scale);
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, &model[0][0]);
    glUniform3f(glGetUniformLocation(shader, "objectColor"), color.r, color.g, color.b);
    glUniform1f(glGetUniformLocation(shader, "emissive"), 0.0f);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

Game::Game(int w, int h)
    : width_(w), height_(h),
      camera_(glm::vec3(0, 10, 20), 35.0f, -35.0f, 0.0f),
      terrain_(128, 128, 100.0f, 15.0f) {}

bool Game::init() {
    if (!glfwInit()) { fprintf(stderr, "GLFW init failed\n"); return false; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(width_, height_, "TankWar3D", nullptr, nullptr);
    if (!window_) { glfwTerminate(); return false; }
    glfwMakeContextCurrent(window_);
    glfwSetFramebufferSizeCallback(window_, fb_size_cb);
    glfwSetScrollCallback(window_, scroll_cb);
    glfwSetMouseButtonCallback(window_, mouse_btn_cb);
    glfwSetCursorPosCallback(window_, mouse_move_cb);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { fprintf(stderr, "GLEW init failed\n"); return false; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    printf("OpenGL %s\n", glGetString(GL_VERSION));

    terrain_shader_ = create_program("shaders/terrain.vert", "shaders/terrain.frag");
    model_shader_ = create_program("shaders/model.vert", "shaders/model.frag");
    particle_shader_ = create_program("shaders/particle.vert", "shaders/particle.frag");
    hud_shader_ = create_program("shaders/hud.vert", "shaders/hud.frag");
    water_shader_ = create_program("shaders/water.vert", "shaders/water.frag");

    g_camera = &camera_;

    terrain_.generate();
    terrain_.upload_gpu();

    weapons_.push_back({"Cannon", 35.0f, 9.81f, 25.0f, 3.0f, WeaponType::Cannon});
    weapons_.push_back({"Missile", 28.0f, 9.81f, 45.0f, 4.0f, WeaponType::Missile});
    weapons_.push_back({"Nuke", 22.0f, 9.81f, 90.0f, 7.0f, WeaponType::Nuke});

    TankConfig cfg;
    tanks_.push_back(Tank(glm::vec3(-22, 0, -5), glm::vec4(0.2f, 0.55f, 1.0f, 1.0f), cfg));
    tanks_[0].name = "Player"; tanks_[0].is_player = true;
    tanks_.push_back(Tank(glm::vec3(22, 0, 5), glm::vec4(1.0f, 0.25f, 0.15f, 1.0f), cfg));
    tanks_[1].name = "CPU"; tanks_[1].heading = 180;

    for (auto& t : tanks_) t.pos.y = terrain_.get_height(t.pos.x, t.pos.z);

    particles_.upload_gpu();
    font_.init();
    init_batched_hud();
    init_water();

    wind_ = glm::vec2(rand_float(-4.0f, 4.0f), rand_float(-4.0f, 4.0f));
    state_ = GameState::Menu;

    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    return true;
}

void Game::run() {
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window_)) {
        double now = glfwGetTime();
        float dt = std::min((float)(now - last), 0.05f);
        last = now;
        game_time_ += dt;
        glfwPollEvents();
        process_input(dt);
        update(dt);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render();
        glfwSwapBuffers(window_);
    }
}

void Game::process_input(float dt) {
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window_, true);

    if (state_ == GameState::Menu) {
        if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_ENTER) == GLFW_PRESS) {
            state_ = GameState::Playing;
            current_tank_ = 0;
        }
        return;
    }

    if (state_ == GameState::GameOver) {
        if (glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS) {
            reset_game();
            state_ = GameState::Menu;
        }
        return;
    }

    if (state_ == GameState::Playing && tanks_[current_tank_].is_player) {
        Tank& t = tanks_[current_tank_];
        t.move_input = 0; t.turn_input = 0;
        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS) t.turn_input = -1;
        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) t.turn_input = 1;
        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) t.move_input = 1;
        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) t.move_input = -1;
        if (glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS)
            t.barrel_angle = std::min(t.barrel_angle + 55.0f * dt, 80.0f);
        if (glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS)
            t.barrel_angle = std::max(t.barrel_angle - 55.0f * dt, -10.0f);
        if (glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS)
            t.power = std::max(t.power - 45.0f * dt, 10.0f);
        if (glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS)
            t.power = std::min(t.power + 45.0f * dt, 100.0f);
        if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS && !projectile_.in_flight)
            fire_projectile();
        static bool tab_was = false;
        if (glfwGetKey(window_, GLFW_KEY_TAB) == GLFW_PRESS && !tab_was)
            current_weapon_ = (current_weapon_ + 1) % (int)weapons_.size();
        tab_was = (glfwGetKey(window_, GLFW_KEY_TAB) == GLFW_PRESS);
    }
}

void Game::update(float dt) {
    camera_.update_shake(dt);

    if (state_ == GameState::Menu || state_ == GameState::GameOver) {
        title_blink_ += dt;
        return;
    }

    for (auto& t : tanks_) {
        if (!t.is_alive()) continue;

        if (!t.is_player && &t == &tanks_[current_tank_] && !projectile_.in_flight) {
            t.move_input = 0; t.turn_input = 0;

            for (auto& o : tanks_) {
                if (o.is_player && o.is_alive()) {
                    float dx = o.pos.x - t.pos.x;
                    float dz = o.pos.z - t.pos.z;
                    float d = std::sqrt(dx * dx + dz * dz);

                    // Ballistic aiming: compute optimal angle for distance
                    glm::vec3 target = o.pos + glm::vec3(0, 1.5f, 0);
                    t.aim_towards(target, terrain_);

                    // Weapon selection by range
                    if (d < 20.0f) current_weapon_ = 0;
                    else if (d < 40.0f) current_weapon_ = 1;
                    else current_weapon_ = 2;

                    // Compute power from ballistic formula: v = sqrt(d*g / sin(2*angle))
                    float angle_rad = glm::radians(t.barrel_angle);
                    float g = weapons_[current_weapon_].gravity;
                    float sin2a = std::sin(2.0f * std::max(angle_rad, 0.1f));
                    float needed_speed = std::sqrt(std::max(1.0f, d * g / std::max(sin2a, 0.01f)));
                    t.power = std::clamp(needed_speed / weapons_[current_weapon_].speed * 50.0f,
                                        15.0f, 95.0f);
                    // Add randomness
                    t.power += rand_float(-5.0f, 5.0f);
                    t.power = std::clamp(t.power, 15.0f, 95.0f);

                    // Movement AI
                    float optDist = 25.0f + rand_float(-5.0f, 10.0f);
                    if (d > optDist + 3) t.move_input = 0.5f;
                    else if (d < optDist - 5) t.move_input = -0.3f;

                    break;
                }
            }
            ai_delay_ += dt;
            if (ai_delay_ > 1.5f + rand_float(0.0f, 0.5f)) {
                fire_projectile();
                ai_delay_ = 0;
            }
        }
        t.update(dt, terrain_);
    }

    if (projectile_.in_flight) {
        particles_.emit_trail(projectile_.position(), projectile_.velocity());
        if (projectile_.update(dt, terrain_, 128, 128, 100.0f))
            check_explosion();
    }

    if (state_ == GameState::Playing && current_tank_ < (int)tanks_.size() && tanks_[current_tank_].is_alive())
        camera_.follow(tanks_[current_tank_].pos, dt);
    else if (state_ == GameState::Watching && projectile_.in_flight)
        camera_.follow(projectile_.position() + glm::vec3(0, 2, 0), dt);

    particles_.update(dt);
    if (flash_timer_ > 0) flash_timer_ -= dt;
}

void Game::fire_projectile() {
    Tank& t = tanks_[current_tank_];
    glm::vec3 muzzle = t.get_muzzle_position();
    glm::vec3 dir = t.get_fire_direction();
    float speed = weapons_[current_weapon_].speed * (t.power / 50.0f);
    glm::vec3 vel = dir * speed + glm::vec3(wind_.x * 0.3f, 0, wind_.y * 0.3f);
    projectile_ = Projectile(muzzle, vel, weapons_[current_weapon_]);
    state_ = GameState::Watching;
    printf("[%s] Fired %s! Power:%.0f%% Angle:%.0f\n",
           t.name.c_str(), weapons_[current_weapon_].name.c_str(), t.power, t.barrel_angle);
}

void Game::check_explosion() {
    glm::vec3 impact = projectile_.position();
    Weapon& w = projectile_.weapon;
    terrain_.destroy_at(impact, w.explosion_radius);

    particles_.emit_explosion(impact, glm::vec3(1, 0.6, 0.1), (int)(w.damage * 3));
    particles_.emit_explosion(impact + glm::vec3(0, 1, 0), glm::vec3(1, 0.3, 0), (int)(w.damage * 1.5));
    particles_.emit_smoke(impact, 25);

    flash_timer_ = 0.3f;
    camera_.add_shake(w.explosion_radius * 0.3f);

    for (auto& t : tanks_) {
        if (t.is_alive()) {
            float d = glm::length(t.pos - impact);
            if (d < w.explosion_radius * 2.5f) {
                float dmg = w.damage * (1.0f - d / (w.explosion_radius * 2.5f));
                t.take_damage(dmg);
                printf("[%s] Hit! -%.0f HP (%.0f remaining)\n", t.name.c_str(), dmg, t.health);
            }
        }
    }

    int alive = 0;
    for (auto& t : tanks_) if (t.is_alive()) alive++;
    if (alive <= 1) {
        state_ = GameState::GameOver;
        printf("GAME OVER!\n");
    } else {
        next_turn();
        state_ = GameState::Playing;
    }
}

void Game::next_turn() {
    int start = current_tank_;
    do { current_tank_ = (current_tank_ + 1) % (int)tanks_.size(); }
    while (!tanks_[current_tank_].is_alive() && current_tank_ != start);
    wind_ = glm::vec2(rand_float(-4.0f, 4.0f), rand_float(-4.0f, 4.0f));
    turn_timer_ = 0;
    ai_delay_ = 0;
    turn_count_++;
    printf("\n=== %s's turn === Wind:(%.1f, %.1f)\n",
           tanks_[current_tank_].name.c_str(), wind_.x, wind_.y);
}

void Game::reset_game() {
    terrain_.generate();
    terrain_.upload_gpu();
    TankConfig cfg;
    tanks_[0] = Tank(glm::vec3(-22, 0, -5), glm::vec4(0.2f, 0.55f, 1.0f, 1.0f), cfg);
    tanks_[0].name = "Player"; tanks_[0].is_player = true;
    tanks_[1] = Tank(glm::vec3(22, 0, 5), glm::vec4(1.0f, 0.25f, 0.15f, 1.0f), cfg);
    tanks_[1].name = "CPU"; tanks_[1].heading = 180;
    for (auto& t : tanks_) t.pos.y = terrain_.get_height(t.pos.x, t.pos.z);
    projectile_ = Projectile();
    current_weapon_ = 0;
    current_tank_ = 0;
    turn_count_ = 0;
    wind_ = glm::vec2(rand_float(-4.0f, 4.0f), rand_float(-4.0f, 4.0f));
}

// ========== BATCHED HUD ==========

void Game::init_batched_hud() {
    glGenVertexArrays(1, &hud_vao_);
    glGenBuffers(1, &hud_vbo_);
    glBindVertexArray(hud_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, hud_vbo_);
    glBufferData(GL_ARRAY_BUFFER, MAX_HUD_VERTS * sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    hud_vert_count_ = 0;
}

void Game::begin_hud_batch() {
    hud_vert_count_ = 0;
}

void Game::push_hud_rect(float x, float y, float w, float h, glm::vec4 color) {
    if (hud_vert_count_ + 6 > MAX_HUD_VERTS) return;
    float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
    float verts[] = {
        x, y,     u0, v0,
        x+w, y,   u1, v0,
        x+w, y+h, u1, v1,
        x, y,     u0, v0,
        x+w, y+h, u1, v1,
        x, y+h,   u0, v1,
    };

    glUseProgram(hud_shader_);
    glUniform2f(glGetUniformLocation(hud_shader_, "resolution"), (float)width_, (float)height_);
    glUniform1i(glGetUniformLocation(hud_shader_, "useTexture"), 0);
    glUniform4fv(glGetUniformLocation(hud_shader_, "color"), 1, &color[0]);

    glBindVertexArray(hud_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, hud_vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Game::push_hud_rect_gradient(float x, float y, float w, float h, glm::vec4 color, float pct) {
    push_hud_rect(x, y, w, h, glm::vec4(0, 0, 0, color.a * 0.5f));
    push_hud_rect(x, y, w * pct, h, color);
}

void Game::flush_hud_batch() {
    glBindVertexArray(0);
}

// ========== WATER ==========

void Game::init_water() {
    float s = 60.0f;
    float verts[] = {
        -s, 0, -s,  0, 1, 0,  0, 0,
         s, 0, -s,  0, 1, 0,  1, 0,
         s, 0,  s,  0, 1, 0,  1, 1,
        -s, 0, -s,  0, 1, 0,  0, 0,
         s, 0,  s,  0, 1, 0,  1, 1,
        -s, 0,  s,  0, 1, 0,  0, 1,
    };
    glGenVertexArrays(1, &water_vao_);
    glGenBuffers(1, &water_vbo_);
    glBindVertexArray(water_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, water_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void Game::draw_water() {
    glUseProgram(water_shader_);
    float aspect = (float)width_ / (float)height_;
    glm::mat4 view = camera_.view_matrix();
    glm::mat4 proj = camera_.projection_matrix(aspect);
    glm::vec3 cam_pos = camera_.position();
    glm::vec3 sun_dir = glm::normalize(glm::vec3(0.6f, 0.8f, 0.3f));

    glUniformMatrix4fv(glGetUniformLocation(water_shader_, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(water_shader_, "projection"), 1, GL_FALSE, &proj[0][0]);
    glUniform3fv(glGetUniformLocation(water_shader_, "cameraPos"), 1, &cam_pos[0]);
    glUniform1f(glGetUniformLocation(water_shader_, "time"), game_time_);
    glUniform3fv(glGetUniformLocation(water_shader_, "sunDirection"), 1, &sun_dir[0]);
    glUniform1f(glGetUniformLocation(water_shader_, "waterLevel"), 0.0f);

    glBindVertexArray(water_vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// ========== RENDER ==========

void Game::render() {
    if (state_ == GameState::Menu) {
        render_menu();
        return;
    }
    render_3d_scene();
    render_hud();
    if (state_ == GameState::GameOver)
        render_game_over();
}

void Game::render_3d_scene() {
    float aspect = (float)width_ / (float)height_;
    glm::mat4 view = camera_.view_matrix();
    glm::mat4 proj = camera_.projection_matrix(aspect);
    glm::vec3 cam_pos = camera_.position();
    glm::vec3 sun_dir = glm::normalize(glm::vec3(0.6f, 0.8f, 0.3f));
    glm::vec3 fog_color(0.53f, 0.81f, 0.92f);
    float fog_density = 0.004f;

    // Flash
    if (flash_timer_ > 0) {
        glClearColor(1.0f, 0.9f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    }

    // Terrain
    glUseProgram(terrain_shader_);
    glUniformMatrix4fv(glGetUniformLocation(terrain_shader_, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(terrain_shader_, "projection"), 1, GL_FALSE, &proj[0][0]);
    glUniform3fv(glGetUniformLocation(terrain_shader_, "sunDirection"), 1, &sun_dir[0]);
    glUniform3f(glGetUniformLocation(terrain_shader_, "sunColor"), 1.0f, 0.95f, 0.85f);
    glUniform3fv(glGetUniformLocation(terrain_shader_, "fogColor"), 1, &fog_color[0]);
    glUniform1f(glGetUniformLocation(terrain_shader_, "fogDensity"), fog_density);
    glUniform3fv(glGetUniformLocation(terrain_shader_, "cameraPos"), 1, &cam_pos[0]);
    terrain_.render();

    // Water
    draw_water();

    // Models
    glUseProgram(model_shader_);
    glUniformMatrix4fv(glGetUniformLocation(model_shader_, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(model_shader_, "projection"), 1, GL_FALSE, &proj[0][0]);
    glUniform3fv(glGetUniformLocation(model_shader_, "lightDir"), 1, &sun_dir[0]);
    glUniform3f(glGetUniformLocation(model_shader_, "lightColor"), 1.0f, 0.95f, 0.85f);
    glUniform3fv(glGetUniformLocation(model_shader_, "cameraPos"), 1, &cam_pos[0]);
    glUniform3fv(glGetUniformLocation(model_shader_, "fogColor"), 1, &fog_color[0]);
    glUniform1f(glGetUniformLocation(model_shader_, "fogDensity"), fog_density);

    for (int i = 0; i < (int)tanks_.size(); i++) {
        const Tank& t = tanks_[i];
        if (!t.is_alive()) continue;
        float h = glm::radians(t.heading);

        // Tracks
        for (float side : {-0.55f, 0.55f}) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.15f, 0));
            model = glm::rotate(model, h, glm::vec3(0, 1, 0));
            model = glm::translate(model, glm::vec3(side, 0, 0));
            model = glm::scale(model, glm::vec3(0.3f, 0.3f, 1.2f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), 0.15f, 0.15f, 0.15f);
            glUniform1f(glGetUniformLocation(model_shader_, "emissive"), 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Body
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.45f, 0));
            model = glm::rotate(model, h, glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(1.0f, 0.35f, 0.7f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), t.color.r, t.color.g, t.color.b);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Turret
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.7f, 0));
            model = glm::rotate(model, h, glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(0.5f, 0.3f, 0.5f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"),
                t.color.r * 0.85f, t.color.g * 0.85f, t.color.b * 0.85f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Barrel
        {
            float barRad = glm::radians(t.barrel_angle);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.75f, 0));
            model = glm::rotate(model, h, glm::vec3(0, 1, 0));
            model = glm::rotate(model, barRad, glm::vec3(1, 0, 0));
            model = glm::translate(model, glm::vec3(0, 0, 0.8f));
            model = glm::scale(model, glm::vec3(0.12f, 0.12f, 1.6f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), 0.3f, 0.3f, 0.3f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        // Shadow
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.02f, 0));
            model = glm::scale(model, glm::vec3(1.5f, 0.01f, 1.2f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), 0.0f, 0.0f, 0.0f);
            glUniform1f(glGetUniformLocation(model_shader_, "emissive"), 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }

    // Projectile
    if (projectile_.in_flight) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, projectile_.position());
        model = glm::scale(model, glm::vec3(0.2f));
        glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
        glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), 1.0f, 0.9f, 0.2f);
        glUniform1f(glGetUniformLocation(model_shader_, "emissive"), 0.8f);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glUniform1f(glGetUniformLocation(model_shader_, "emissive"), 0.0f);
    }

    // Trajectory preview (dotted line using small cubes)
    if (state_ == GameState::Playing && tanks_[current_tank_].is_player && !projectile_.in_flight) {
        const Tank& t = tanks_[current_tank_];
        const Weapon& w = weapons_[current_weapon_];
        glm::vec3 muzzle = t.get_muzzle_position();
        glm::vec3 dir = t.get_fire_direction();
        float speed = w.speed * (t.power / 50.0f);
        glm::vec3 vel = dir * speed + glm::vec3(wind_.x * 0.3f, 0, wind_.y * 0.3f);
        glm::vec3 pp = muzzle;
        glm::vec3 pv = vel;
        for (int i = 0; i < 120; i++) {
            pv.y -= w.gravity * 0.05f;
            pp += pv * 0.05f;
            if (pp.y < terrain_.get_height(pp.x, pp.z)) break;
            if (i % 3 == 0) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, pp);
                model = glm::scale(model, glm::vec3(0.08f));
                glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
                float fade = 1.0f - (float)i / 120.0f;
                glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), 1.0f, 1.0f, 1.0f);
                glUniform1f(glGetUniformLocation(model_shader_, "emissive"), fade * 0.5f);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
        glUniform1f(glGetUniformLocation(model_shader_, "emissive"), 0.0f);
    }
    glBindVertexArray(0);

    // Particles
    particles_.render(view, proj, camera_.right(), camera_.up(), particle_shader_);
}

void Game::render_hud() {
    glDisable(GL_DEPTH_TEST);

    begin_hud_batch();
    float pad = 15.0f;
    float bw = 220.0f;
    float bh = 20.0f;

    const Tank& t = tanks_[current_tank_];

    // Bottom-left panel background
    push_hud_rect(pad - 5, (float)height_ - pad - 105, bw + 10, 110, glm::vec4(0, 0, 0, 0.5f));

    // Health bar
    push_hud_rect(pad, (float)height_ - pad - 22, bw, bh, glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
    float hp = t.health / 100.0f;
    glm::vec4 hp_color = (hp > 0.5f) ? glm::vec4(0.1f, 0.85f, 0.2f, 1.0f)
                         : (hp > 0.25f) ? glm::vec4(1.0f, 0.8f, 0.0f, 1.0f)
                         : glm::vec4(1.0f, 0.15f, 0.1f, 1.0f);
    push_hud_rect(pad, (float)height_ - pad - 22, bw * hp, bh, hp_color);

    // Power bar
    push_hud_rect(pad, (float)height_ - pad - 47, bw, bh, glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
    push_hud_rect(pad, (float)height_ - pad - 47, bw * (t.power / 100.0f), bh,
                  glm::vec4(0.2f, 0.5f, 1.0f, 1.0f));

    // Weapon indicator
    glm::vec4 wpn_color = weapons_[current_weapon_].type == WeaponType::Cannon ? glm::vec4(1.0f, 0.7f, 0.0f, 1.0f)
                         : weapons_[current_weapon_].type == WeaponType::Missile ? glm::vec4(1.0f, 0.4f, 0.1f, 1.0f)
                         : glm::vec4(0.8f, 0.1f, 0.8f, 1.0f);
    push_hud_rect(pad, (float)height_ - pad - 72, bw, bh, glm::vec4(0.15f, 0.15f, 0.15f, 0.6f));
    push_hud_rect(pad + 3, (float)height_ - pad - 69, 70.0f, 14.0f, wpn_color);

    // Wind panel (top right)
    push_hud_rect((float)width_ - pad - bw - 5, pad, bw + 10, 50, glm::vec4(0, 0, 0, 0.4f));

    // Wind arrow
    float wind_len = glm::length(wind_);
    float wnx = (wind_len > 0.1f) ? wind_.x / wind_len : 0;
    float wny = (wind_len > 0.1f) ? wind_.y / wind_len : 0;
    float acx = (float)width_ - pad - bw * 0.5f - 5;
    float acy = pad + 25;
    push_hud_rect(acx - 2, acy - 2, 4, 4, glm::vec4(1, 1, 1, 0.8f));
    push_hud_rect(acx + wnx * 25 - 3, acy - wny * 25 - 3, 6, 6, glm::vec4(0.5f, 0.8f, 1.0f, 0.9f));

    // Turn panel (top center)
    push_hud_rect((float)width_ * 0.5f - 80, pad, 160, 30, glm::vec4(0, 0, 0, 0.5f));

    flush_hud_batch();

    // Text rendering
    float text_y = (float)height_ - pad - 18;
    font_.draw_text("HP", pad + 3, text_y, 1.2f, glm::vec4(1, 1, 1, 0.9f), hud_shader_, width_, height_);
    font_.draw_text(std::to_string((int)t.health) + "/100", pad + 35, text_y, 1.2f, glm::vec4(1, 1, 1, 0.9f), hud_shader_, width_, height_);

    font_.draw_text("PWR", pad + 3, text_y - 25, 1.2f, glm::vec4(1, 1, 1, 0.9f), hud_shader_, width_, height_);
    font_.draw_text(std::to_string((int)t.power) + "%", pad + 45, text_y - 25, 1.2f, glm::vec4(0.5f, 0.8f, 1, 1), hud_shader_, width_, height_);

    font_.draw_text(weapons_[current_weapon_].name, pad + 80, text_y - 50, 1.3f, wpn_color, hud_shader_, width_, height_);

    font_.draw_text("WIND " + std::to_string((int)wind_len), (float)width_ - pad - bw + 5, pad + 5, 1.1f, glm::vec4(0.6f, 0.85f, 1, 0.9f), hud_shader_, width_, height_);

    // Turn text
    if (state_ == GameState::Playing) {
        glm::vec4 turn_color = tanks_[current_tank_].is_player ? glm::vec4(0, 1, 0.3f, 1) : glm::vec4(1, 1, 0, 1);
        std::string turn_text = tanks_[current_tank_].is_player ? "YOUR TURN" : "CPU THINKING...";
        float tw = font_.text_width(turn_text, 1.4f);
        font_.draw_text(turn_text, (float)width_ * 0.5f - tw * 0.5f, pad + 7, 1.4f, turn_color, hud_shader_, width_, height_);
    } else if (state_ == GameState::Watching) {
        font_.draw_text("INCOMING...", (float)width_ * 0.5f - 50, pad + 7, 1.4f, glm::vec4(1, 0.6f, 0, 1), hud_shader_, width_, height_);
    }

    // Controls help
    font_.draw_text("WASD:Move  Arrows:Aim/Power  Space:Fire  Tab:Weapon", 10, 8, 0.8f, glm::vec4(1, 1, 1, 0.3f), hud_shader_, width_, height_);

    glEnable(GL_DEPTH_TEST);
}

void Game::render_menu() {
    glClearColor(0.08f, 0.08f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    begin_hud_batch();
    float cx = (float)width_ * 0.5f;
    float cy = (float)height_ * 0.5f;

    // Background
    push_hud_rect(0, 0, (float)width_, (float)height_, glm::vec4(0.05f, 0.05f, 0.1f, 1.0f));

    // Title bar
    push_hud_rect(cx - 300, cy - 120, 600, 240, glm::vec4(0.1f, 0.12f, 0.2f, 0.9f));
    push_hud_rect(cx - 280, cy - 95, 560, 3, glm::vec4(0.3f, 0.7f, 1.0f, 0.5f));
    push_hud_rect(cx - 280, cy + 50, 560, 3, glm::vec4(0.3f, 0.7f, 1.0f, 0.5f));

    // Tank icons
    push_hud_rect(cx - 260, cy - 30, 25, 14, glm::vec4(0.2f, 0.55f, 1.0f, 0.8f));
    push_hud_rect(cx - 258, cy - 20, 10, 7, glm::vec4(0.15f, 0.4f, 0.8f, 0.8f));
    push_hud_rect(cx - 252, cy - 18, 18, 4, glm::vec4(0.3f, 0.3f, 0.3f, 0.8f));

    push_hud_rect(cx + 235, cy - 30, 25, 14, glm::vec4(1.0f, 0.25f, 0.15f, 0.8f));
    push_hud_rect(cx + 238, cy - 20, 10, 7, glm::vec4(0.8f, 0.2f, 0.1f, 0.8f));
    push_hud_rect(cx + 234, cy - 18, 18, 4, glm::vec4(0.3f, 0.3f, 0.3f, 0.8f));

    // Start button
    float alpha = 0.4f + 0.6f * std::abs(std::sin(title_blink_ * 3.0f));
    push_hud_rect(cx - 160, cy + 55, 320, 35, glm::vec4(0.2f, 0.6f, 1.0f, alpha));

    flush_hud_batch();

    font_.draw_text("TANK WAR 3D", cx - font_.text_width("TANK WAR 3D", 3.5f) * 0.5f, cy - 80, 3.5f,
                     glm::vec4(0.3f, 0.8f, 1.0f, 1.0f), hud_shader_, width_, height_);
    font_.draw_text("Turn-Based Tank Battle", cx - font_.text_width("Turn-Based Tank Battle", 1.5f) * 0.5f, cy - 35, 1.5f,
                     glm::vec4(0.5f, 0.7f, 0.8f, 1.0f), hud_shader_, width_, height_);
    font_.draw_text("[PRESS SPACE TO START]", cx - font_.text_width("[PRESS SPACE TO START]", 1.6f) * 0.5f, cy + 62, 1.6f,
                     glm::vec4(1, 1, 1, alpha), hud_shader_, width_, height_);

    font_.draw_text("WASD:Move  Arrows:Aim  Space:Fire  Tab:Weapon", cx - font_.text_width("WASD:Move  Arrows:Aim  Space:Fire  Tab:Weapon", 0.9f) * 0.5f, cy + 120, 0.9f,
                     glm::vec4(0.4f, 0.4f, 0.5f, 0.7f), hud_shader_, width_, height_);

    glEnable(GL_DEPTH_TEST);
}

void Game::render_game_over() {
    glDisable(GL_DEPTH_TEST);

    begin_hud_batch();
    float cx = (float)width_ * 0.5f;
    float cy = (float)height_ * 0.5f;

    push_hud_rect(0, 0, (float)width_, (float)height_, glm::vec4(0, 0, 0, 0.65f));
    push_hud_rect(cx - 280, cy - 120, 560, 240, glm::vec4(0.08f, 0.08f, 0.15f, 0.95f));

    int winner = -1;
    for (int i = 0; i < (int)tanks_.size(); i++)
        if (tanks_[i].is_alive()) winner = i;

    if (winner >= 0) {
        glm::vec4 wc = tanks_[winner].color;
        push_hud_rect(cx - 220, cy - 80, 440, 50, glm::vec4(wc.r, wc.g, wc.b, 0.9f));
        push_hud_rect(cx - 215, cy - 75, 430, 40, glm::vec4(0.08f, 0.08f, 0.12f, 1.0f));
    }

    push_hud_rect(cx - 200, cy + 10, 400, 30, glm::vec4(0.2f, 0.2f, 0.3f, 0.5f));

    float alpha = 0.4f + 0.6f * std::abs(std::sin(game_time_ * 3.0f));
    push_hud_rect(cx - 140, cy + 55, 280, 35, glm::vec4(0.2f, 0.8f, 0.3f, alpha));

    flush_hud_batch();

    std::string result = (winner >= 0 && tanks_[winner].is_player) ? "VICTORY!" : "DEFEATED!";
    glm::vec4 result_color = (winner >= 0 && tanks_[winner].is_player) ? glm::vec4(0.2f, 1, 0.4f, 1) : glm::vec4(1, 0.3f, 0.2f, 1);
    font_.draw_text(result, cx - font_.text_width(result, 3.0f) * 0.5f, cy - 70, 3.0f,
                     result_color, hud_shader_, width_, height_);

    std::string stats = "Turns: " + std::to_string(turn_count_);
    font_.draw_text(stats, cx - font_.text_width(stats, 1.2f) * 0.5f, cy + 15, 1.2f,
                     glm::vec4(0.6f, 0.6f, 0.7f, 0.8f), hud_shader_, width_, height_);

    font_.draw_text("[PRESS R TO PLAY AGAIN]", cx - font_.text_width("[PRESS R TO PLAY AGAIN]", 1.5f) * 0.5f, cy + 62, 1.5f,
                     glm::vec4(1, 1, 1, alpha), hud_shader_, width_, height_);

    glEnable(GL_DEPTH_TEST);
}

void Game::shutdown() {
    font_.cleanup();
    if (hud_vao_) glDeleteVertexArrays(1, &hud_vao_);
    if (hud_vbo_) glDeleteBuffers(1, &hud_vbo_);
    if (water_vao_) glDeleteVertexArrays(1, &water_vao_);
    if (water_vbo_) glDeleteBuffers(1, &water_vbo_);
    if (cube_vao_) glDeleteVertexArrays(1, &cube_vao_);
    if (cube_vbo_) glDeleteBuffers(1, &cube_vbo_);
    glfwDestroyWindow(window_);
    glfwTerminate();
}
