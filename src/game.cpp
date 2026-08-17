#include "game.h"
#include "utils.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstring>

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

static void draw_cube() {
    static GLuint vao = 0, vbo = 0;
    if (vao == 0) {
        float v[] = {
            -0.5f,-0.5f, 0.5f,  0, 0, 1,  0.5f,-0.5f, 0.5f,  0, 0, 1,  0.5f, 0.5f, 0.5f,  0, 0, 1,
            -0.5f,-0.5f, 0.5f,  0, 0, 1,  0.5f, 0.5f, 0.5f,  0, 0, 1, -0.5f, 0.5f, 0.5f,  0, 0, 1,
             0.5f,-0.5f,-0.5f,  0, 0,-1, -0.5f,-0.5f,-0.5f,  0, 0,-1, -0.5f, 0.5f,-0.5f,  0, 0,-1,
             0.5f,-0.5f,-0.5f,  0, 0,-1, -0.5f, 0.5f,-0.5f,  0, 0,-1,  0.5f, 0.5f,-0.5f,  0, 0,-1,
            -0.5f, 0.5f, 0.5f,  0, 1, 0,  0.5f, 0.5f, 0.5f,  0, 1, 0,  0.5f, 0.5f,-0.5f,  0, 1, 0,
            -0.5f, 0.5f, 0.5f,  0, 1, 0,  0.5f, 0.5f,-0.5f,  0, 1, 0, -0.5f, 0.5f,-0.5f,  0, 1, 0,
            -0.5f,-0.5f,-0.5f,  0,-1, 0,  0.5f,-0.5f,-0.5f,  0,-1, 0,  0.5f,-0.5f, 0.5f,  0,-1, 0,
            -0.5f,-0.5f,-0.5f,  0,-1, 0,  0.5f,-0.5f, 0.5f,  0,-1, 0, -0.5f,-0.5f, 0.5f,  0,-1, 0,
             0.5f,-0.5f, 0.5f,  1, 0, 0,  0.5f,-0.5f,-0.5f,  1, 0, 0,  0.5f, 0.5f,-0.5f,  1, 0, 0,
             0.5f,-0.5f, 0.5f,  1, 0, 0,  0.5f, 0.5f,-0.5f,  1, 0, 0,  0.5f, 0.5f, 0.5f,  1, 0, 0,
            -0.5f,-0.5f,-0.5f, -1, 0, 0, -0.5f,-0.5f, 0.5f, -1, 0, 0, -0.5f, 0.5f, 0.5f, -1, 0, 0,
            -0.5f,-0.5f,-0.5f, -1, 0, 0, -0.5f, 0.5f, 0.5f, -1, 0, 0, -0.5f, 0.5f,-0.5f, -1, 0, 0,
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

Game::Game(int w, int h)
    : width_(w), height_(h),
      camera_(glm::vec3(0, 10, 20), 35.0f, 35.0f, 0.0f),
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
    tanks_[1].name = "AI Bot"; tanks_[1].heading = 180;

    for (auto& t : tanks_) t.pos.y = terrain_.get_height(t.pos.x, t.pos.z);

    particles_.upload_gpu();
    init_hud_geometry();
    init_water();
    init_tank_geometry();

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
            state_ = GameState::Menu;
            terrain_.generate();
            terrain_.upload_gpu();
            TankConfig cfg;
            tanks_[0] = Tank(glm::vec3(-22, 0, -5), glm::vec4(0.2f, 0.55f, 1.0f, 1.0f), cfg);
            tanks_[0].name = "Player"; tanks_[0].is_player = true;
            tanks_[1] = Tank(glm::vec3(22, 0, 5), glm::vec4(1.0f, 0.25f, 0.15f, 1.0f), cfg);
            tanks_[1].name = "AI Bot"; tanks_[1].heading = 180;
            for (auto& t : tanks_) t.pos.y = terrain_.get_height(t.pos.x, t.pos.z);
            projectile_ = Projectile();
            current_weapon_ = 0;
            current_tank_ = 0;
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
                    float d = glm::length(glm::vec2(t.pos.x - o.pos.x, t.pos.z - o.pos.z));

                    glm::vec3 target = o.pos + glm::vec3(0, 1.5f, 0);
                    t.aim_towards(target, terrain_);

                    if (d > 30.0f) t.move_input = 0.6f;
                    else if (d > 15.0f) t.move_input = 0.2f;
                    else if (d < 8.0f) t.move_input = -0.5f;
                    else {
                        float cross = (t.pos.x - o.pos.x) * sin(glm::radians(o.heading))
                                    - (t.pos.z - o.pos.z) * cos(glm::radians(o.heading));
                        t.turn_input = (cross > 0) ? 0.3f : -0.3f;
                    }

                    if (d < 20.0f) current_weapon_ = 0;
                    else if (d < 35.0f) current_weapon_ = 1;
                    else current_weapon_ = 2;

                    t.power = std::clamp(40.0f + d * 1.2f + rand_float(-8, 8), 20.0f, 95.0f);
                    break;
                }
            }
            ai_delay_ += dt;
            if (ai_delay_ > 1.5f) {
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

    if (state_ == GameState::Playing && tanks_[current_tank_].is_alive())
        camera_.follow(tanks_[current_tank_].pos, dt);

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
    printf("\n=== %s's turn === Wind:(%.1f, %.1f)\n",
           tanks_[current_tank_].name.c_str(), wind_.x, wind_.y);
}

void Game::init_hud_geometry() {
    float verts[] = {
        0, 0,  0, 0,
        1, 0,  1, 0,
        1, 1,  1, 1,
        0, 0,  0, 0,
        1, 1,  1, 1,
        0, 1,  0, 1,
    };
    glGenVertexArrays(1, &hud_vao_);
    glGenBuffers(1, &hud_vbo_);
    glBindVertexArray(hud_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, hud_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void Game::draw_hud_rect(float x, float y, float w, float h, glm::vec4 color) {
    glUseProgram(hud_shader_);
    glUniform2f(glGetUniformLocation(hud_shader_, "resolution"), (float)width_, (float)height_);
    glUniform1i(glGetUniformLocation(hud_shader_, "useTexture"), 0);
    glUniform4fv(glGetUniformLocation(hud_shader_, "color"), 1, &color[0]);

    float data[] = { x, y, x + w, y, x + w, y + h, x, y, x + w, y + h, x, y + h };
    GLuint tmpVAO, tmpVBO;
    glGenVertexArrays(1, &tmpVAO);
    glGenBuffers(1, &tmpVBO);
    glBindVertexArray(tmpVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tmpVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDeleteBuffers(1, &tmpVBO);
    glDeleteVertexArrays(1, &tmpVAO);
}

void Game::draw_hud_rect_gradient(float x, float y, float w, float h, glm::vec4 color, float fill_pct) {
    draw_hud_rect(x, y, w, h, glm::vec4(0, 0, 0, color.a * 0.5f));
    draw_hud_rect(x, y, w * fill_pct, h, color);
}

void Game::init_water() {
    float s = 60.0f;
    float h = 0.0f;
    float verts[] = {
        -s, h, -s,  0, 1, 0,
         s, h, -s,  0, 1, 0,
         s, h,  s,  0, 1, 0,
        -s, h, -s,  0, 1, 0,
         s, h,  s,  0, 1, 0,
        -s, h,  s,  0, 1, 0,
    };
    glGenVertexArrays(1, &water_vao_);
    glGenBuffers(1, &water_vbo_);
    glBindVertexArray(water_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, water_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
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

    glBindVertexArray(water_vao_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Game::init_tank_geometry() {
    float v[] = {
        -0.5f,-0.5f, 0.5f,  0, 0, 1,  0.5f,-0.5f, 0.5f,  0, 0, 1,  0.5f, 0.5f, 0.5f,  0, 0, 1,
        -0.5f,-0.5f, 0.5f,  0, 0, 1,  0.5f, 0.5f, 0.5f,  0, 0, 1, -0.5f, 0.5f, 0.5f,  0, 0, 1,
         0.5f,-0.5f,-0.5f,  0, 0,-1, -0.5f,-0.5f,-0.5f,  0, 0,-1, -0.5f, 0.5f,-0.5f,  0, 0,-1,
         0.5f,-0.5f,-0.5f,  0, 0,-1, -0.5f, 0.5f,-0.5f,  0, 0,-1,  0.5f, 0.5f,-0.5f,  0, 0,-1,
        -0.5f, 0.5f, 0.5f,  0, 1, 0,  0.5f, 0.5f, 0.5f,  0, 1, 0,  0.5f, 0.5f,-0.5f,  0, 1, 0,
        -0.5f, 0.5f, 0.5f,  0, 1, 0,  0.5f, 0.5f,-0.5f,  0, 1, 0, -0.5f, 0.5f,-0.5f,  0, 1, 0,
        -0.5f,-0.5f,-0.5f,  0,-1, 0,  0.5f,-0.5f,-0.5f,  0,-1, 0,  0.5f,-0.5f, 0.5f,  0,-1, 0,
        -0.5f,-0.5f,-0.5f,  0,-1, 0,  0.5f,-0.5f, 0.5f,  0,-1, 0, -0.5f,-0.5f, 0.5f,  0,-1, 0,
         0.5f,-0.5f, 0.5f,  1, 0, 0,  0.5f,-0.5f,-0.5f,  1, 0, 0,  0.5f, 0.5f,-0.5f,  1, 0, 0,
         0.5f,-0.5f, 0.5f,  1, 0, 0,  0.5f, 0.5f,-0.5f,  1, 0, 0,  0.5f, 0.5f, 0.5f,  1, 0, 0,
        -0.5f,-0.5f,-0.5f, -1, 0, 0, -0.5f,-0.5f, 0.5f, -1, 0, 0, -0.5f, 0.5f, 0.5f, -1, 0, 0,
        -0.5f,-0.5f,-0.5f, -1, 0, 0, -0.5f, 0.5f, 0.5f, -1, 0, 0, -0.5f, 0.5f,-0.5f, -1, 0, 0,
    };
    tank_cube_verts_ = 36;
    glGenVertexArrays(1, &tank_vao_);
    glGenBuffers(1, &tank_vbo_);
    glBindVertexArray(tank_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, tank_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

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

    // Screen flash
    if (flash_timer_ > 0) {
        float intensity = flash_timer_ / 0.3f;
        glClearColor(1.0f, 0.9f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
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

    glBindVertexArray(tank_vao_);
    for (int i = 0; i < (int)tanks_.size(); i++) {
        const Tank& t = tanks_[i];
        if (!t.is_alive()) continue;

        float headRad = glm::radians(t.heading);

        // Tracks (left + right)
        for (float side : {-0.55f, 0.55f}) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.15f, 0));
            model = glm::rotate(model, headRad, glm::vec3(0, 1, 0));
            model = glm::translate(model, glm::vec3(side, 0, 0));
            model = glm::scale(model, glm::vec3(0.3f, 0.3f, 1.2f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), 0.15f, 0.15f, 0.15f);
            glUniform1f(glGetUniformLocation(model_shader_, "emissive"), 0.0f);
            glDrawArrays(GL_TRIANGLES, 0, tank_cube_verts_);
        }

        // Body
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.45f, 0));
            model = glm::rotate(model, headRad, glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(1.0f, 0.35f, 0.7f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), t.color.r, t.color.g, t.color.b);
            glDrawArrays(GL_TRIANGLES, 0, tank_cube_verts_);
        }

        // Turret
        {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.7f, 0));
            model = glm::rotate(model, headRad, glm::vec3(0, 1, 0));
            model = glm::scale(model, glm::vec3(0.5f, 0.3f, 0.5f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"),
                t.color.r * 0.85f, t.color.g * 0.85f, t.color.b * 0.85f);
            glDrawArrays(GL_TRIANGLES, 0, tank_cube_verts_);
        }

        // Barrel
        {
            float barRad = glm::radians(t.barrel_angle);
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, t.pos + glm::vec3(0, 0.75f, 0));
            model = glm::rotate(model, headRad, glm::vec3(0, 1, 0));
            model = glm::rotate(model, barRad, glm::vec3(1, 0, 0));
            model = glm::translate(model, glm::vec3(0, 0, 0.8f));
            model = glm::scale(model, glm::vec3(0.12f, 0.12f, 1.6f));
            glUniformMatrix4fv(glGetUniformLocation(model_shader_, "model"), 1, GL_FALSE, &model[0][0]);
            glUniform3f(glGetUniformLocation(model_shader_, "objectColor"), 0.3f, 0.3f, 0.3f);
            glDrawArrays(GL_TRIANGLES, 0, tank_cube_verts_);
        }

        // Health bar above tank
        if (i == current_tank_) {
            float barW = 1.8f, barH = 0.12f;
            float health_pct = t.health / 100.0f;
            glm::vec4 bg(0, 0, 0, 0.7f);
            glm::vec4 fg = (health_pct > 0.5f) ? glm::vec4(0.1f, 0.9f, 0.2f, 0.9f)
                         : (health_pct > 0.25f) ? glm::vec4(1.0f, 0.8f, 0.0f, 0.9f)
                         : glm::vec4(1.0f, 0.2f, 0.1f, 0.9f);
            draw_hud_rect_gradient(
                (t.pos.x / 50.0f + 0.5f) * width_ * 0.5f + width_ * 0.25f - barW * 0.5f * 30.0f,
                80.0f, barW * 30.0f, barH * 30.0f, fg, health_pct);
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
        glDrawArrays(GL_TRIANGLES, 0, tank_cube_verts_);
        glUniform1f(glGetUniformLocation(model_shader_, "emissive"), 0.0f);
    }
    glBindVertexArray(0);

    // Particles
    particles_.render(view, proj, camera_.right(), camera_.up(), particle_shader_);
}

void Game::render_hud() {
    glDisable(GL_DEPTH_TEST);

    float pad = 15.0f;
    float bar_w = 200.0f;
    float bar_h = 18.0f;

    // Background panel
    draw_hud_rect(pad - 5, (float)height_ - pad - 85, bar_w + 10, 85, glm::vec4(0, 0, 0, 0.4f));

    const Tank& t = tanks_[current_tank_];

    // Health bar
    draw_hud_rect(pad, (float)height_ - pad - 25, bar_w, bar_h, glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
    float hp = t.health / 100.0f;
    glm::vec4 hp_color = (hp > 0.5f) ? glm::vec4(0.1f, 0.85f, 0.2f, 1.0f)
                        : (hp > 0.25f) ? glm::vec4(1.0f, 0.8f, 0.0f, 1.0f)
                        : glm::vec4(1.0f, 0.15f, 0.1f, 1.0f);
    draw_hud_rect(pad, (float)height_ - pad - 25, bar_w * hp, bar_h, hp_color);

    // Power bar
    draw_hud_rect(pad, (float)height_ - pad - 50, bar_w, bar_h, glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
    draw_hud_rect(pad, (float)height_ - pad - 50, bar_w * (t.power / 100.0f), bar_h,
                  glm::vec4(0.2f, 0.5f, 1.0f, 1.0f));

    // Weapon name
    draw_hud_rect(pad, (float)height_ - pad - 75, bar_w, bar_h, glm::vec4(0.15f, 0.15f, 0.15f, 0.6f));
    draw_hud_rect(pad + 3, (float)height_ - pad - 72, 60.0f, 12.0f,
                  weapons_[current_weapon_].type == WeaponType::Cannon ? glm::vec4(0.4f, 0.8f, 1.0f, 1.0f)
                  : weapons_[current_weapon_].type == WeaponType::Missile ? glm::vec4(1.0f, 0.6f, 0.2f, 1.0f)
                  : glm::vec4(1.0f, 0.2f, 0.1f, 1.0f));

    // Wind indicator (top right)
    draw_hud_rect((float)width_ - pad - bar_w - 5, pad, bar_w + 10, 55, glm::vec4(0, 0, 0, 0.4f));

    float wind_len = glm::length(wind_);
    float wind_nx = (wind_len > 0.1f) ? wind_.x / wind_len : 0;
    float wind_ny = (wind_len > 0.1f) ? wind_.y / wind_len : 0;
    float arrow_cx = (float)width_ - pad - bar_w * 0.5f - 5;
    float arrow_cy = pad + 28;
    float arrow_len = 25.0f;
    draw_hud_rect(arrow_cx - 2, arrow_cy - 2, 4, 4, glm::vec4(1, 1, 1, 0.8f));
    draw_hud_rect(arrow_cx + wind_nx * arrow_len - 3, arrow_cy - wind_ny * arrow_len - 3, 6, 6,
                  glm::vec4(0.5f, 0.8f, 1.0f, 0.9f));

    // Turn indicator
    draw_hud_rect(pad, pad, bar_w + 10, 30, glm::vec4(0, 0, 0, 0.5f));

    glEnable(GL_DEPTH_TEST);
}

void Game::render_menu() {
    glClearColor(0.08f, 0.08f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    float cx = (float)width_ * 0.5f;
    float cy = (float)height_ * 0.5f;

    // Title background
    draw_hud_rect(cx - 300, cy - 120, 600, 240, glm::vec4(0.1f, 0.12f, 0.2f, 0.9f));

    // Title
    draw_hud_rect(cx - 250, cy - 80, 500, 60, glm::vec4(0.2f, 0.6f, 1.0f, 1.0f));
    draw_hud_rect(cx - 240, cy - 70, 480, 40, glm::vec4(0.1f, 0.1f, 0.18f, 1.0f));

    // Decorative elements
    draw_hud_rect(cx - 250, cy - 100, 500, 3, glm::vec4(0.3f, 0.7f, 1.0f, 0.5f));
    draw_hud_rect(cx - 250, cy + 20, 500, 3, glm::vec4(0.3f, 0.7f, 1.0f, 0.5f));

    // Tank icon (left)
    draw_hud_rect(cx - 280, cy - 30, 20, 12, glm::vec4(0.2f, 0.55f, 1.0f, 0.8f));
    draw_hud_rect(cx - 278, cy - 22, 8, 6, glm::vec4(0.15f, 0.4f, 0.8f, 0.8f));
    draw_hud_rect(cx - 274, cy - 20, 15, 3, glm::vec4(0.3f, 0.3f, 0.3f, 0.8f));

    // Tank icon (right)
    draw_hud_rect(cx + 260, cy - 30, 20, 12, glm::vec4(1.0f, 0.25f, 0.15f, 0.8f));
    draw_hud_rect(cx + 264, cy - 22, 8, 6, glm::vec4(0.8f, 0.2f, 0.1f, 0.8f));
    draw_hud_rect(cx + 259, cy - 20, 15, 3, glm::vec4(0.3f, 0.3f, 0.3f, 0.8f));

    // Start prompt (blinking)
    float alpha = 0.5f + 0.5f * std::sin(title_blink_ * 3.0f);
    draw_hud_rect(cx - 150, cy + 50, 300, 35, glm::vec4(0.2f, 0.6f, 1.0f, alpha));

    // Instructions
    draw_hud_rect(cx - 200, cy + 110, 400, 3, glm::vec4(0.3f, 0.3f, 0.5f, 0.3f));

    glEnable(GL_DEPTH_TEST);
}

void Game::render_game_over() {
    glDisable(GL_DEPTH_TEST);

    float cx = (float)width_ * 0.5f;
    float cy = (float)height_ * 0.5f;

    draw_hud_rect(0, 0, (float)width_, (float)height_, glm::vec4(0, 0, 0, 0.6f));
    draw_hud_rect(cx - 250, cy - 100, 500, 200, glm::vec4(0.1f, 0.1f, 0.18f, 0.95f));

    // Winner indicator
    int winner = -1;
    for (int i = 0; i < (int)tanks_.size(); i++)
        if (tanks_[i].is_alive()) winner = i;

    if (winner >= 0) {
        glm::vec4 wc = tanks_[winner].color;
        draw_hud_rect(cx - 200, cy - 70, 400, 50, glm::vec4(wc.r, wc.g, wc.b, 0.9f));
        draw_hud_rect(cx - 195, cy - 65, 390, 40, glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));
    }

    draw_hud_rect(cx - 180, cy + 10, 360, 30, glm::vec4(0.3f, 0.3f, 0.5f, 0.5f));

    // Restart prompt
    float alpha = 0.5f + 0.5f * std::sin(game_time_ * 3.0f);
    draw_hud_rect(cx - 120, cy + 55, 240, 30, glm::vec4(0.2f, 0.8f, 0.3f, alpha));

    glEnable(GL_DEPTH_TEST);
}

void Game::shutdown() {
    if (hud_vao_) glDeleteVertexArrays(1, &hud_vao_);
    if (hud_vbo_) glDeleteBuffers(1, &hud_vbo_);
    if (water_vao_) glDeleteVertexArrays(1, &water_vao_);
    if (water_vbo_) glDeleteBuffers(1, &water_vbo_);
    if (tank_vao_) glDeleteVertexArrays(1, &tank_vao_);
    if (tank_vbo_) glDeleteBuffers(1, &tank_vbo_);
    glfwDestroyWindow(window_);
    glfwTerminate();
}
