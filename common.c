/*
    common.c
    Code shared between all programs.

    Benedict Henshaw
    Jan 2018
*/

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <stdint.h>

// Custom primitive types.
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef _Bool bool;
#define true 1
#define false 0

// All possible entities in the game.
#define FLOOR  1
#define WALL   2
#define SPIKES 3
#define EXIT   4
#define LOCK   5
#define GOLD   6
#define KEY    7
#define ENEMY  8
#define PLAYER 9
#define ENTITY_TYPE_COUNT 9

// All entities that are unaffected by updates.
#define STATIC_ENTITIES (BIT(FLOOR) | BIT(WALL) | BIT(SPIKES) | BIT(EXIT))
// All entities that cannot be walked on.
#define SOLID_ENTITIES (BIT(WALL))

// Levels are made of tiles, each of which is a bit array.
typedef u16 Tile;
#define LEVEL_SIZE 22
typedef Tile Level[LEVEL_SIZE * LEVEL_SIZE];

// The width and height of each tile in pixels.
#define SPRITE_SIZE 32

// Directions that the player can move.
enum { UP = 1, DOWN, LEFT, RIGHT };

// Bit selector.
#define BIT(n) (1 << (n))

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)
#define CLAMP(low, val, high) MIN(MAX(low, val), high)

#define sq(x) ((x) * (x))

u64 random_seed[2] = { (u64)__DATE__, (u64)__TIME__ };
// Xoroshiro128+ pseudo-random number generator.
u64 random_u64() {
    u64 s0 = random_seed[0];
    u64 s1 = random_seed[1];
    u64 result = s0 + s1;
    s1 ^= s0;
    #define LS(x, k) ((x << k) | (x >> (64 - k)))
    random_seed[0] = LS(s0, 55) ^ s1 ^ (s1 << 14);
    random_seed[1] = LS(s1, 36);
    #undef LS
    return result;
}

// Set the seed for the pseudo-random number generator.
void set_seed(u64 a, u64 b) {
    random_seed[0] ^= a;
    random_seed[1] ^= b;
    for (int i = 0; i < 64; ++i) random_u64();
}

// Get a random float between 0.0 and 1.0.
float random_float() {
    return (float)random_u64() / (float)UINT64_MAX;
}

// Get a random int between low and high, inclusive.
int random_int_range(int low, int high) {
    int d = abs(high - low) + 1;
    return random_float() * d + low;
}

// Get a random boolean.
bool chance(float chance_to_be_true) {
    return random_float() <= chance_to_be_true;
}

// Calls a function for all tiles that are touched by a basic four-way flood fill
// starting at tile x,y. You can pass data into the given function using 'data'.
// 'mask' allows you to set which entity bits to check when flooding.
// 'target' allows you to set the values of those bits. Other bits will be ignored.
typedef bool (*Flood_Func)(Tile *, int x, int y, void *);
int flood(Level level, int start_x, int start_y, Tile mask, Tile target, Flood_Func func, void * data) {
    enum { TODO = 1, DONE = 2 };
    u8 todo[LEVEL_SIZE * LEVEL_SIZE] = {0};
    int steps_taken = 0;

    todo[start_x + start_y * LEVEL_SIZE] = TODO;

    // This loop could be unbounded, but there is a known maximum which is
    // visiting every single tile in the level.
    for (int i = 0; i < LEVEL_SIZE * LEVEL_SIZE; ++i) {
        // If a whole iteration takes place and no tiles marked TODO are found, then
        // we know that the flood is finished.
        bool cleared_todo = true;
        for (int y = 0; y < LEVEL_SIZE; ++y) {
            for (int x = 0; x < LEVEL_SIZE; ++x) {
                if (todo[x + y * LEVEL_SIZE] == TODO) {
                    // We found a tile marked TODO, so we are not finished.
                    cleared_todo = false;

                    // Check the masked bits of the tile, and see if they match the target bits.
                    if ((level[x + y * LEVEL_SIZE] & mask) == (target & mask)) {
                        bool stop = func(&level[x + y * LEVEL_SIZE], x, y, data);
                        if (stop) return steps_taken;

                        todo[x + y * LEVEL_SIZE] = DONE;

                        // Add the neighbouring tiles, if they have not already been checked.
                        if (x > 0 && todo[(x - 1) + y * LEVEL_SIZE] != DONE) {
                            todo[(x - 1) + y * LEVEL_SIZE] = TODO;
                        }
                        if (x < LEVEL_SIZE - 1 && todo[(x + 1) + y * LEVEL_SIZE] != DONE) {
                            todo[(x + 1) + y * LEVEL_SIZE] = TODO;
                        }
                        if (y > 0 && todo[x + (y - 1) * LEVEL_SIZE] != DONE) {
                            todo[x + (y - 1) * LEVEL_SIZE] = TODO;
                        }
                        if (y < LEVEL_SIZE - 1 && todo[x + (y + 1) * LEVEL_SIZE] != DONE) {
                            todo[x + (y + 1) * LEVEL_SIZE] = TODO;
                        }

                        ++steps_taken;
                    }
                }
            }
        }

        if (cleared_todo) break;
    }

    return steps_taken;
}

// Passed into flood, it will record all the entity types that were flooded.
// It takes the address of a Tile in data, where it will record the results.
bool flood_record_tiles(Tile * tile, int x, int y, void * data) {
    Tile * result = data;
    *result |= *tile;
    return false;
}

bool find_player(Level level, int * px, int * py) {
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            if (level[x + y * LEVEL_SIZE] & BIT(PLAYER)) {
                *px = x;
                *py = y;
                return true;
            }
        }
    }
    return false;
}

bool level_is_completable(Level level) {
    int px, py;
    if (!find_player(level, &px, &py)) return false;

    Tile entities_seen = 0;
    flood(level, px, py,
        BIT(FLOOR) | BIT(WALL) | BIT(SPIKES), BIT(FLOOR),
        flood_record_tiles, &entities_seen);

    return (entities_seen & (BIT(KEY) | BIT(EXIT))) == (BIT(KEY) | BIT(EXIT));
}

// Load a level from binary '.lvl' file.
bool load_level(FILE * file, Level level) {
    return (bool)fread(level, sizeof(Tile), LEVEL_SIZE * LEVEL_SIZE, file);
}

// Level is written out as binary format in native byte ordering.
bool write_level(FILE * file, Level level) {
    return (bool)fwrite(level, sizeof(Tile), LEVEL_SIZE * LEVEL_SIZE, file);
}

void print_ascii_level(Level level) {
    char entity_chars[] = " _#^E%*KEP";
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            Tile tile = level[x + y * LEVEL_SIZE];
            int top_entity = 0;
            for (int bit = 1; bit <= ENTITY_TYPE_COUNT; ++bit) {
                if (tile & BIT(bit)) top_entity = bit;
            }
            printf("%c", entity_chars[top_entity]);
        }
        printf("\n");
    }
}
