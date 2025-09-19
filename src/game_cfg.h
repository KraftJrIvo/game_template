#include <array>
#include <vector>

#include "raylib.h"

#define WIN_NOM "g a m e"

#define RAND_FLOAT static_cast <float> (rand()) / (static_cast <float> (RAND_MAX) + 1.0f)
#define RAND_FLOAT_SIGNED (2.0f * RAND_FLOAT - 1.0f)
#define RAND_FLOAT_SIGNED_2D Vector2{RAND_FLOAT_SIGNED, RAND_FLOAT_SIGNED}

#define WINDOW_WIDTH   512
#define WINDOW_HEIGHT  512

#define COLORS std::array<Color, 5>{ RED, GREEN, BLUE, GOLD, PINK }

#define SPRITE_SPEED 1000.0f