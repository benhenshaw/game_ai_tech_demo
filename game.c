/*
    game.c
    Simple tile-based game to demonstrate level generation.

    Benedict Henshaw
    Jan 2018
*/

// This program uses SDL2 to be cross-platform.
#include <SDL2/SDL.h>
#include "common.c"

// Used to get graphics on screen.
SDL_Window * window = NULL;
SDL_Renderer * renderer = NULL;
SDL_Texture * sprite_texture = NULL;

// Stats from playing the game
int gold_collected = 0;
int enemies_killed = 0;
int steps_taken = 0;

// Draws a sprite using its ID.
// The coordinates correspond to tiles, not pixels.
void draw_sprite(int entity_id, int tile_x, int tile_y) {
    SDL_Rect sprite_rect = {
        entity_id * SPRITE_SIZE, 0,
        SPRITE_SIZE, SPRITE_SIZE
    };
    SDL_Rect screen_rect = {
        tile_x * SPRITE_SIZE, tile_y * SPRITE_SIZE,
        SPRITE_SIZE, SPRITE_SIZE
    };
    SDL_RenderCopy(renderer, sprite_texture, &sprite_rect, &screen_rect);
}

// Draws an entire level.
void draw_level(Level level) {
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            Tile tile = level[x + y * LEVEL_SIZE];
            // Check each bit of the tile, and draw the entity if
            // the corresponding bit is set.
            for (int bit = 1; bit <= ENTITY_TYPE_COUNT; ++bit) {
                if (tile & BIT(bit)) draw_sprite(bit, x, y);
            }
        }
    }
}

// Updates the entire level, stepping the player in the given direction.
// Returns true if the game has ended for any reason.
bool update_level(Level level, int direction) {
    bool game_over = false;

    // Create an empty level that will replace the old one.
    Level updated_level = {0};

    // Copy over all entities that are unaffected by updates.
    for (int i = 0; i < LEVEL_SIZE * LEVEL_SIZE; ++i) {
        updated_level[i] |= level[i] & STATIC_ENTITIES;
    }

    // If a key still exists in the stage, then it has not been found.
    bool key_has_been_collected = true;

    // First pass through level handles dynamic entities.
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            Tile tile = level[x + y * LEVEL_SIZE];

            if (tile & BIT(ENEMY)) {
                // Move the enemy in a random direction.
                int new_x = x, new_y = y;
                int direction = random_int_range(1, 6);
                if (direction == UP)    --new_y; else
                if (direction == DOWN)  ++new_y; else
                if (direction == LEFT)  --new_x; else
                if (direction == RIGHT) ++new_x;

                // Only succeed if that tile is not solid.
                if ((level[new_x + new_y * LEVEL_SIZE] & SOLID_ENTITIES) == 0) {
                    updated_level[new_x + new_y * LEVEL_SIZE] |= BIT(ENEMY);
                } else {
                    updated_level[x + y * LEVEL_SIZE] |= BIT(ENEMY);
                }
            }

            if (tile & BIT(PLAYER)) {
                // If the player is already standing on a spider, kill it.
                if (updated_level[x + y * LEVEL_SIZE] & BIT(ENEMY)) {
                    updated_level[x + y * LEVEL_SIZE] ^= BIT(ENEMY);
                    ++enemies_killed;
                }

                // Attempt to move to the new tile.
                int new_x = x, new_y = y;
                if (direction == UP)    --new_y; else
                if (direction == DOWN)  ++new_y; else
                if (direction == LEFT)  --new_x; else
                if (direction == RIGHT) ++new_x;

                Tile new_tile = level[new_x + new_y * LEVEL_SIZE];

                // Only succeed if that tile is not solid.
                if ((new_tile & SOLID_ENTITIES) == 0) {
                    updated_level[new_x + new_y * LEVEL_SIZE] |= BIT(PLAYER);

                    // Collect any collectables on the new tile.
                    if (new_tile & BIT(GOLD)) {
                        level[new_x + new_y * LEVEL_SIZE] ^= BIT(GOLD);
                        ++gold_collected;
                    }
                    if (new_tile & BIT(KEY)) {
                        level[new_x + new_y * LEVEL_SIZE] ^= BIT(KEY);
                    }

                    // Kill any enemies on the new tile.
                    if (new_tile & BIT(ENEMY)) {
                        level[new_x + new_y * LEVEL_SIZE] ^= BIT(ENEMY);
                        ++enemies_killed;
                    }

                    // End the game if the player reaches the exit and there
                    // is not a lock on it.
                    if (new_tile & BIT(EXIT) && (new_tile & BIT(LOCK)) == 0) {
                        game_over = true;
                    }

                    // End the game if the player steps onto spikes.
                    if (new_tile & BIT(SPIKES)) {
                        game_over = true;
                    }
                } else {
                    // Place the player onto their original tile in the updated
                    // level if they did not move in any direction.
                    updated_level[x + y * LEVEL_SIZE] |= BIT(PLAYER);
                }
            }
        }
    }

    // Second pass handles collectables, as they may have been affected by the first pass.
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            Tile tile = level[x + y * LEVEL_SIZE];
            if (tile & BIT(GOLD)) updated_level[x + y * LEVEL_SIZE] |= BIT(GOLD);
            if (tile & BIT(KEY)) {
                key_has_been_collected = false;
                updated_level[x + y * LEVEL_SIZE] |= BIT(KEY);
            }
        }
    }

    // Final pass handles the lock.
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            Tile tile = level[x + y * LEVEL_SIZE];
            if (tile & BIT(LOCK) && !key_has_been_collected) {
                updated_level[x + y * LEVEL_SIZE] |= BIT(LOCK);
            }
        }
    }

    // Overwrite the original level with the update.
    for (int i = 0; i < LEVEL_SIZE * LEVEL_SIZE; ++i) {
        level[i] = updated_level[i];
    }

    // The update is fully carried out, even when it is known earlier on that
    // the player has done something to end the game, to make sure the player
    // sees the final state of the game represented on screen at the end.
    return game_over;
}

int main(int argument_count, char ** arguments) {
    // Load a level.
    Level level = {0};
    load_level(stdin, level);

    // Set up everything needed for graphics.
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        LEVEL_SIZE * SPRITE_SIZE, LEVEL_SIZE * SPRITE_SIZE, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    sprite_texture = SDL_CreateTextureFromSurface(renderer,
        SDL_LoadBMP("sheet.bmp"));

    // Seed the random number generator.
    set_seed(~SDL_GetTicks(), ~SDL_GetPerformanceCounter());

    bool game_over = false;

    // Begin the frame loop.
    while (!game_over) {
        // Parse all events that have occurred.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) game_over = true;
            if (event.type == SDL_KEYDOWN) {
                SDL_Scancode sc = event.key.keysym.scancode;
                // If the player pressed a movement key take a step in that direction,
                // updating the rest of the level too.
                int direction = 0;
                if (sc == SDL_SCANCODE_UP    || sc == SDL_SCANCODE_W) direction = UP;
                if (sc == SDL_SCANCODE_DOWN  || sc == SDL_SCANCODE_S) direction = DOWN;
                if (sc == SDL_SCANCODE_LEFT  || sc == SDL_SCANCODE_A) direction = LEFT;
                if (sc == SDL_SCANCODE_RIGHT || sc == SDL_SCANCODE_D) direction = RIGHT;
                // Set direction to an arbitrary non-zero value to cause an update,
                // but not cause any movement to occur within update_level.
                if (sc == SDL_SCANCODE_SPACE) direction = ~0;
                if (direction) {
                    game_over = update_level(level, direction);
                    ++steps_taken;
                }
            }
        }
        SDL_RenderClear(renderer);
        draw_level(level);
        SDL_Delay(10);
        SDL_RenderPresent(renderer);
    }

    // The game is now over, so show a message with the results.
    char message[512];
    snprintf(message, 512,
        "Thanks for playing!\n"
        "\n"
        "Stats:\n"
        "Gold Collected: %d\n"
        "Enemies Killed: %d\n"
        "Steps Taken: %d",
        gold_collected,
        enemies_killed,
        steps_taken
    );

    // Show it as a pop-up box.
    SDL_ShowSimpleMessageBox(0, "Game Over!", message, window);

    // And print it to stdout.
    printf("Game Over!\n%s\n", message);
}
