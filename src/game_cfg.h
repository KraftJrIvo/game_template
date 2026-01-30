#include <array>
#include <vector>

#include "raylib.h"

#define WIN_NOM "g a m e"

#define RAND_FLOAT static_cast <float> (rand()) / (static_cast <float> (RAND_MAX) + 1.0f)
#define RAND_FLOAT_SIGNED (2.0f * RAND_FLOAT - 1.0f)
#define RAND_FLOAT_SIGNED_2D Vector2{RAND_FLOAT_SIGNED, RAND_FLOAT_SIGNED}

#define GAME_WIDTH  768
#define GAME_HEIGHT 432
#define GAME_SCALE std::max(std::min(floor(GetScreenWidth() / float(GAME_WIDTH)), floor(GetScreenHeight() / float(GAME_HEIGHT))), 1.0f)

#define GAME_SCALE_MAX float(int(GetMonitorHeight(GetCurrentMonitor())) / GAME_HEIGHT)
#define GAME_TOP GAME_HEIGHT * (0.5f * (1.0f - (GetScreenHeight() / (GAME_SCALE * GAME_HEIGHT))))
#define GAME_LEFT   GAME_WIDTH * (0.5f * (1.0f - (GetScreenWidth() / (GAME_SCALE * GAME_WIDTH))))
#define GAME_BOTTOM GAME_HEIGHT * (1.0f - 0.5f * (1.0f - (GetScreenHeight() / (GAME_SCALE * GAME_HEIGHT))))
#define GAME_RIGHT   GAME_WIDTH * (1.0f - 0.5f * (1.0f - (GetScreenWidth() / (GAME_SCALE * GAME_WIDTH))))

#define WINDOW_WIDTH  GAME_WIDTH
#define WINDOW_HEIGHT GAME_HEIGHT

#define COLORS std::array<Color, 5>{ RED, GREEN, BLUE, GOLD, PINK }

#define SPEED 1000.0f
#define MAX_PLAYERS 4