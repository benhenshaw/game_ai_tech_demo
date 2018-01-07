/*
    editor.c
    A simple level editor for the tile-based game.

    Benedict Henshaw
    Jan 2018
*/

#include <SDL2/SDL.h>
#include "common.c"

SDL_Window * window = NULL;
SDL_Renderer * renderer = NULL;
SDL_Texture * sprite_texture = NULL;

void draw_sprite(int entity_index, int tile_x, int tile_y) {
    SDL_Rect sprite_rect = {
        entity_index * SPRITE_SIZE, 0,
        SPRITE_SIZE, SPRITE_SIZE
    };
    SDL_Rect screen_rect = {
        tile_x * SPRITE_SIZE, tile_y * SPRITE_SIZE,
        SPRITE_SIZE, SPRITE_SIZE
    };
    SDL_RenderCopy(renderer, sprite_texture, &sprite_rect, &screen_rect);
}

// Draws a number on screen using a bitmap font.
void draw_number(int number, int screen_x, int screen_y) {
    char number_string[64];
    snprintf(number_string, 64, "%d", number);
    for (int i = 0; i < 64 && number_string[i]; ++i) {
        int offset = SPRITE_SIZE * 10;
        int char_offset = (number_string[i] - '0') * 8;
        SDL_Rect sprite_rect = { offset + char_offset, 0, 8, 8 };
        SDL_Rect screen_rect = { screen_x + i * 8, screen_y, 8, 8 };
        SDL_RenderCopy(renderer, sprite_texture, &sprite_rect, &screen_rect);
    }
}

void draw_level(Level level) {
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            Tile tile = level[x + y * LEVEL_SIZE];
            for (int bit = 1; bit <= ENTITY_TYPE_COUNT; ++bit) {
                if (tile & BIT(bit)) draw_sprite(bit, x, y);
            }
        }
    }
}

void fill_level(Level level, int entity) {
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            level[x + y * LEVEL_SIZE] = BIT(entity);
        }
    }
}

void draw_rect(Level level, int entity, int ax, int ay, int bx, int by) {
    for (int x = ax; x < bx; ++x) {
        level[x +  0 * LEVEL_SIZE] = BIT(entity);
        level[x + bx * LEVEL_SIZE] = BIT(entity);
    }

    for (int y = ay; y < by; ++y) {
        level[0  + y * LEVEL_SIZE] = BIT(entity);
        level[by + y * LEVEL_SIZE] = BIT(entity);
    }
}

void fill_spikes(Tile * tile, int x, int y, void * data) {
    *tile |= BIT(SPIKES);
}

bool verify_level(Tile * tile, int x, int y, void * data) {
    struct { bool key, exit; } * found = data;
    *tile |= BIT(SPIKES);
    if (*tile & BIT(KEY))  found->key = true;
    if (*tile & BIT(EXIT)) found->exit = true;
    // Stop early if we have found what we need.
    return found->key && found->exit;
}

bool is_key(Tile * tile, int x, int y, void * data) {
    *tile |= BIT(SPIKES);
    return *tile & BIT(KEY);
}

int main(int argument_count, char ** arguments) {
    Level level = {0};
    // load_level(stdin, level);

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        LEVEL_SIZE * SPRITE_SIZE, LEVEL_SIZE * SPRITE_SIZE, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    sprite_texture = SDL_CreateTextureFromSurface(renderer, SDL_LoadBMP("sheet.bmp"));

    int tx = 0;
    int ty = 0;
    int tile_type = WALL;
    int steps = 0;

    while (true) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) return 0;
            if (event.type == SDL_KEYDOWN) {
                SDL_Scancode sc = event.key.keysym.scancode;
                if (sc == SDL_SCANCODE_BACKSPACE) {
                    fill_level(level, FLOOR);
                } else if (sc == SDL_SCANCODE_RETURN) {
                    write_level(stdout, level);
                } else if (sc == SDL_SCANCODE_F) {
                    fill_level(level, tile_type);
                } else if (sc == SDL_SCANCODE_Z) {
                    struct { bool key, exit; } found = {0};
                    steps = flood(level, tx, ty, BIT(FLOOR), BIT(FLOOR) | BIT(WALL), verify_level, &found);
                } else if (sc == SDL_SCANCODE_1) {
                    tile_type = 1;
                } else if (sc == SDL_SCANCODE_2) {
                    tile_type = 2;
                } else if (sc == SDL_SCANCODE_3) {
                    tile_type = 3;
                } else if (sc == SDL_SCANCODE_4) {
                    tile_type = 4;
                } else if (sc == SDL_SCANCODE_5) {
                    tile_type = 5;
                } else if (sc == SDL_SCANCODE_6) {
                    tile_type = 6;
                } else if (sc == SDL_SCANCODE_7) {
                    tile_type = 7;
                } else if (sc == SDL_SCANCODE_8) {
                    tile_type = 8;
                } else if (sc == SDL_SCANCODE_9) {
                    tile_type = 9;
                } else if (sc == SDL_SCANCODE_0) {
                    tile_type = 0;
                }
            }
            if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.x;
                int my = event.motion.y;
                tx = mx / SPRITE_SIZE;
                ty = my / SPRITE_SIZE;
                if (event.motion.state == SDL_BUTTON_LEFT) {
                    level[tx + ty * LEVEL_SIZE] = BIT(tile_type);
                } else if (event.motion.state == SDL_BUTTON_RIGHT) {
                    level[tx + ty * LEVEL_SIZE] ^= BIT(tile_type);
                }
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.button == SDL_BUTTON_LEFT) {
                    level[tx + ty * LEVEL_SIZE] = BIT(tile_type);
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    level[tx + ty * LEVEL_SIZE] ^= BIT(tile_type);
                }
            }
            if (event.type == SDL_MOUSEWHEEL) {
                tile_type += (0 < event.wheel.y) - (event.wheel.y < 0);
                tile_type %= ENTITY_TYPE_COUNT;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        draw_level(level);

        SDL_SetTextureAlphaMod(sprite_texture, 150 + 50 * sinf(SDL_GetTicks() * 0.01f));
        draw_sprite(tile_type, tx, ty);
        SDL_SetTextureAlphaMod(sprite_texture, 255);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer,
            &(SDL_Rect){tx*SPRITE_SIZE,ty*SPRITE_SIZE,SPRITE_SIZE,SPRITE_SIZE});

        draw_number(tx, 0, 0);
        draw_number(ty, 32, 0);

        draw_number(level_is_completable(level), 0, 8);
        draw_number(steps, 0, 16);

        SDL_Delay(3);
        SDL_RenderPresent(renderer);
    }
}
