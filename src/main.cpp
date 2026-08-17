#include "game.h"
#include <cstdio>

int main(int argc, char* argv[]) {
    printf("TankWar3D - Physics Tank Battle Game\n");
    printf("Controls:\n");
    printf("  WASD    - Move/turn tank\n");
    printf("  Arrows  - Aim barrel / set power\n");
    printf("  Space   - Fire!\n");
    printf("  Tab     - Switch weapon\n");
    printf("  RMB+Drag - Orbit camera\n");
    printf("  Scroll  - Zoom\n");
    printf("  Esc     - Quit\n\n");
    
    Game game(1280, 720);
    if (!game.init()) {
        fprintf(stderr, "Failed to initialize game!\n");
        return 1;
    }
    
    game.run();
    game.shutdown();
    return 0;
}
