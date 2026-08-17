# TankWar3D

A 3D physics-based tank battle game built with **raw OpenGL 3.3 Core** — no engine, no abstraction layers. Worms/Scorched Earth-style turn-based artillery combat with destructible terrain, multiple weapons, wind physics, water, and particle effects.

![C++](https://img.shields.io/badge/C%2B%2B-17-blue) ![OpenGL](https://img.shields.io/badge/OpenGL-3.3%20Core-orange) ![Build](https://img.shields.io/badge/Build-CMake-green)

## Features

- **Raw OpenGL 3.3 Core** — no game engine, pure graphics API
- **Destructible terrain** — procedural heightmap with crater destruction
- **Turn-based combat** — 1v1 local (Player vs AI Bot)
- **3 weapons** — Cannon (fast), Missile (mid), Nuke (massive radius)
- **Wind system** — random wind affects projectile trajectory each turn
- **Animated water** — wave simulation with Fresnel reflections
- **Particle effects** — billboard explosions, smoke trails, projectile fire trails
- **Multi-part tank models** — body, turret, barrel, tracks (all procedural geometry)
- **Orbital camera** — right-click drag to orbit, scroll to zoom
- **HUD overlay** — health bar, power meter, wind compass, weapon indicator
- **Menu & game over screens** — with winner display and restart
- **Screen flash** — explosion feedback effect
- **AI opponent** — aims, repositions, selects weapons based on range
- **Distance fog** — atmospheric depth effect

## Screenshots

The game renders a procedurally generated terrain with:
- Height-based coloring (water → sand → grass → rock → snow)
- Slope-aware blending (steep cliffs show rock)
- Animated water plane with wave and reflection effects
- Multi-part tanks with health bars, turrets, and barrels

*(Run on a machine with a GPU to see it in action!)*

## Build

### Dependencies

| Library | Version |
|---------|---------|
| C++ compiler | C++17 or later |
| CMake | 3.16+ |
| GLFW | 3.3+ |
| GLEW | 2.0+ |
| GLM | Any recent |
| OpenGL | 3.3+ Core |

### Install (Ubuntu/Debian)
```bash
sudo apt install cmake g++ libglfw3-dev libglew-dev libglm-dev
```

### Install (Fedora)
```bash
sudo dnf install cmake gcc-c++ glfw-devel glew-devel glm-devel
```

### Install (macOS)
```bash
brew install cmake glfw glew glm
```

### Build & Run
```bash
git clone https://github.com/SmartGenzAI1/clip_pro.git
cd clip_pro/tankwar3d
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/tankwar3d
```

## Controls

| Key | Action |
|-----|--------|
| **W/S** | Move forward/backward |
| **A/D** | Turn left/right |
| **Up/Down Arrow** | Adjust barrel angle |
| **Left/Right Arrow** | Adjust power |
| **Space** | Fire! |
| **Tab** | Switch weapon (Cannon → Missile → Nuke) |
| **Right Click + Drag** | Orbit camera |
| **Scroll** | Zoom in/out |
| **Esc** | Quit |
| **R** | Restart (game over screen) |
| **Enter/Space** | Start (menu screen) |

## Architecture

```
tankwar3d/
├── CMakeLists.txt          # Build system
├── README.md
├── include/                # Header-only modules
│   ├── camera.h            # Orbital camera with follow mode
│   ├── game.h              # Main game class declaration
│   ├── particle.h          # Billboard particle system
│   ├── projectile.h        # Ballistic projectile physics
│   ├── stb_image.h         # Image loading (stb)
│   ├── tank.h              # Tank model and controls
│   ├── terrain.h           # Procedural heightmap terrain
│   └── utils.h             # Shader compilation, math utilities
├── shaders/
│   ├── terrain.vert/frag   # Slope-based terrain coloring + fog
│   ├── model.vert/frag     # Phong lighting with fog + emissive
│   ├── particle.vert/frag  # Billboard quad particles
│   ├── water.vert/frag     # Animated water with Fresnel
│   ├── hud.vert/frag       # 2D HUD overlay
│   └── sky.vert/frag       # Sky gradient (unused)
└── src/
    ├── main.cpp             # Entry point
    └── game.cpp             # Game loop, rendering, input, AI
```

### Tech Stack
- **Graphics:** Raw OpenGL 3.3 Core Profile (no engine)
- **Windowing:** GLFW
- **Math:** GLM
- **Extensions:** GLEW
- **Build:** CMake
- **Language:** C++17

## License

Open source. Use however you like.
