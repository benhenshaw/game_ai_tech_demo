/*
    level.c
    Tile-based level generator.
    For Game AI Programming.

    Benedict Henshaw
    Jan 2018
*/

#include "common.c"

typedef struct {
    int x, y, w, h;
} Rect;

// Sets the value of every tile in the level.
// Clears all bits other than the given entity bit.
void fill_level(Level level, int entity) {
    for (int y = 0; y < LEVEL_SIZE; ++y) {
        for (int x = 0; x < LEVEL_SIZE; ++x) {
            level[x + y * LEVEL_SIZE] = BIT(entity);
        }
    }
}

// Produces a floor filled, walled level.
void empty_level(Level level) {
    fill_level(level, FLOOR);

    for (int x = 1; x < LEVEL_SIZE-1; ++x) {
        level[x] = BIT(WALL);
        level[x + (LEVEL_SIZE-1) * LEVEL_SIZE] = BIT(WALL);
    }

    for (int y = 0; y < LEVEL_SIZE; ++y) {
        level[0 + y * LEVEL_SIZE] = BIT(WALL);
        level[(LEVEL_SIZE-1) + y * LEVEL_SIZE] = BIT(WALL);
    }
}

// Very basic level generator using noise.
// Results are unvalidated and often very poor.
void scatter_generator(Level level) {
    float portion_of_level_to_be_floor = 0.5f;

    int floor_count = portion_of_level_to_be_floor * (LEVEL_SIZE * LEVEL_SIZE);

    // Start with just walls.
    fill_level(level, WALL);

    // Scatter some floor around the level.
    for (int i = 0; i < floor_count; ++i) {
        int x = random_int_range(1, LEVEL_SIZE-2);
        int y = random_int_range(1, LEVEL_SIZE-2);
        level[x + y * LEVEL_SIZE] = BIT(FLOOR);
    }
}

// Step-by-step subtractive level generator.
// Every walk-able tile in the resulting level is guaranteed to be accessible.
void digger_generator(Level level, float * parameters) {
    int iterations = 5;
    float turn_chance_step = 0.01f;
    float ideal_walkable_portion = 0.2f;

    if (parameters) {
        // iterations             *= 2 * parameters[0];
        // iterations = MAX(1, iterations);
        turn_chance_step       *= 2 * parameters[1];
        ideal_walkable_portion *= 2 * parameters[2];
    }

    for (int i = 0; i < iterations; ++i) {
        float turn_chance = turn_chance_step;

        // After the first iteration, start the digger at a tile that
        // has already been dug out. This will ensure that all tiles are
        // accessible from any starting point.
        int digger_x, digger_y;
        do {
            digger_x = random_int_range(1, LEVEL_SIZE-1);
            digger_y = random_int_range(1, LEVEL_SIZE-1);
        } while (i > 0 && (level[digger_x + digger_y * LEVEL_SIZE] != BIT(FLOOR)));
        int direction = random_int_range(1, 4);

        int ideal_walkable = ideal_walkable_portion * (LEVEL_SIZE * LEVEL_SIZE);

        for (int count = 0; count < ideal_walkable; ++count) {
            level[digger_x + digger_y * LEVEL_SIZE] &= ~BIT(WALL);
            level[digger_x + digger_y * LEVEL_SIZE] = BIT(FLOOR);

            if (direction == UP)    --digger_y; else
            if (direction == DOWN)  ++digger_y; else
            if (direction == LEFT)  --digger_x; else
            if (direction == RIGHT) ++digger_x;
            digger_x = CLAMP(1, digger_x, LEVEL_SIZE-2);
            digger_y = CLAMP(1, digger_y, LEVEL_SIZE-2);
            if (chance(turn_chance)) direction = random_int_range(1, 4);
            else turn_chance += turn_chance_step;
        }
    }
}

// Places random rectangular rooms with no consideration for intersection.
// Very poor results on its own, but can be used in combination with other
// generators to produce better results.
void basic_room_generator(Level level) {
    int count = 8;
    int min_width = 2;
    int max_width = 6;
    int min_height = 2;
    int max_height = 6;

    for (int i = 0; i < count; ++i) {
        int center_x, center_y;
        do {
            center_x = random_int_range(1, LEVEL_SIZE-1);
            center_y = random_int_range(1, LEVEL_SIZE-1);
        } while ((level[center_x + center_y * LEVEL_SIZE] != BIT(FLOOR)));
        int width  = random_int_range(min_width, max_width);
        int height = random_int_range(min_height, max_height);
        int top_left_x = center_x - width / 2;
        int top_left_y = center_y - height / 2;

        for (int y = 0; y < height; ++y) {
            for (int x = 0 ; x < width; ++x) {
                int tx = top_left_x + x;
                int ty = top_left_y + y;
                tx = CLAMP(1, tx, LEVEL_SIZE-2);
                ty = CLAMP(1, ty, LEVEL_SIZE-2);
                level[tx + ty * LEVEL_SIZE] = BIT(FLOOR);
            }
        }
    }
}

// Takes a level with entities already in it, and adds walls
// at random, verifying that they do not make the level impossible.
void reverse_verified_scatter_generator(Level level) {
    float portion_of_level_to_be_wall = 4.0f;
    int wall_count = portion_of_level_to_be_wall * (LEVEL_SIZE * LEVEL_SIZE);
    int attempts = 32;

    for (int i = 0; i < wall_count; ++i) {
        int x, y;
        // Only attempt to place the wall a few times, as there may not be any
        // more valid locations left.
        for (int i = 0; i < attempts; ++i) {
            // Choose a location.
            x = random_int_range(1, LEVEL_SIZE-2);
            y = random_int_range(1, LEVEL_SIZE-2);

            // If it will overwrite the player, skip it.
            if (level[x + y * LEVEL_SIZE] & BIT(PLAYER)) continue;

            // Put the new wall into the level, but preserve other original bits.
            level[x + y * LEVEL_SIZE] |= BIT(WALL);

            // Test to see if it invalidates the level.
            if (!level_is_completable(level)) {
                // If it does, take it out.
                level[x + y * LEVEL_SIZE] ^= BIT(WALL);
            } else {
                // This is an acceptable place for a wall, so remove the old bits.
                level[x + y * LEVEL_SIZE] = BIT(WALL);

                // Continue on to the next wall tile.
                break;
            }
        }
    }
}

bool count_entities(Tile * tile, int x, int y, void * data) {
    int * count = data;
    if (*tile & ~(BIT(FLOOR) | BIT(WALL) | BIT(SPIKES))) {
        *count += 1;
    }
    return false;
}

// TODO:
void reverse_entity_preserving_scatter_generator(Level level) {
    float portion_of_level_to_be_wall = 0.5f;
    int wall_count = portion_of_level_to_be_wall * (LEVEL_SIZE * LEVEL_SIZE);
    int attempts = 32;

    int actual_entity_count = 0;
    for (int i = 0; i < LEVEL_SIZE * LEVEL_SIZE; ++i) {
        count_entities(&level[i], 0, 0, &actual_entity_count);
    }

    for (int i = 0; i < wall_count; ++i) {
        int x, y;
        // Only attempt to place the wall a few times, as there may not be any
        // more valid locations left.
        for (int i = 0; i < attempts; ++i) {
            // Choose a location.
            x = random_int_range(1, LEVEL_SIZE-2);
            y = random_int_range(1, LEVEL_SIZE-2);

            // If it will overwrite the player, skip it.
            if (level[x + y * LEVEL_SIZE] & BIT(PLAYER)) continue;

            // Put the new wall into the level, but preserve other original bits.
            level[x + y * LEVEL_SIZE] |= BIT(WALL);

            bool all_entities_are_accessible = false;
            int px, py, counted_entities;
            if (!find_player(level, &px, &py)) return;
            flood(level, px, py,
                BIT(FLOOR) | BIT(WALL) | BIT(SPIKES), BIT(FLOOR),
                count_entities, &counted_entities);

            all_entities_are_accessible = (actual_entity_count == counted_entities);

            // Test to see if it invalidates the level.
            if (!all_entities_are_accessible) {
                // If it does, take it out.
                level[x + y * LEVEL_SIZE] ^= BIT(WALL);
            } else {
                // This is an acceptable place for a wall, so remove the old bits.
                level[x + y * LEVEL_SIZE] = BIT(WALL);

                // Continue on to the next wall tile.
                break;
            }
        }
    }
}

// TODO:
void reverse_verified_fill_generator(Level level) {
    for (int y = 1; y < LEVEL_SIZE-1; ++y) {
        for (int x = 1; x < LEVEL_SIZE-1; ++x) {
            // Put the new wall into the level, but preserve other original bits.
            level[x + y * LEVEL_SIZE] |= BIT(WALL);

            // Test to see if it invalidates the level.
            if (!level_is_completable(level)) {
                // If it does, take it out.
                level[x + y * LEVEL_SIZE] ^= BIT(WALL);
            } else {
                // This is an acceptable place for a wall, so remove the old bits.
                level[x + y * LEVEL_SIZE] = BIT(WALL);
                // Continue on to the next wall tile.
                break;
            }
        }
    }
}

// Puts entities in the level at random.
// Very basic and simple results.
// Can produce incompletable levels.
void scatter_placer(Level level, float * parameters) {
    float gold_chance = 0.07f;
    float enemy_chance = 0.03f;
    float spikes_chance = 0.03f;

    // if (parameters) {
    //     gold_chance   *= 2 * parameters[0];
    //     enemy_chance  *= 2 * parameters[1];
    //     spikes_chance *= 2 * parameters[2];
    // }

    // Scatter some entities around.
    for (int y = 1; y < LEVEL_SIZE-1; ++y) {
        for (int x = 1; x < LEVEL_SIZE-1; ++x) {
            Tile t = level[x + y * LEVEL_SIZE];
            if ((t & BIT(WALL)) == 0) {
                if (chance(gold_chance))   t |= BIT(GOLD);   else
                if (chance(enemy_chance))  t |= BIT(ENEMY);  else
                if (chance(spikes_chance)) t |= BIT(SPIKES);
            }
            level[x + y * LEVEL_SIZE] = t;
        }
    }

    int x, y;

    // Place the locked door.
    do {
        x = random_int_range(1, LEVEL_SIZE-1);
        y = random_int_range(1, LEVEL_SIZE-1);
    } while ((level[x + y * LEVEL_SIZE] != BIT(FLOOR)));
    level[x + y * LEVEL_SIZE] = BIT(FLOOR) | BIT(EXIT) | BIT(LOCK);

    // Place the key.
    do {
        x = random_int_range(1, LEVEL_SIZE-1);
        y = random_int_range(1, LEVEL_SIZE-1);
    } while ((level[x + y * LEVEL_SIZE] != BIT(FLOOR)));
    level[x + y * LEVEL_SIZE] = BIT(FLOOR) | BIT(KEY);

    // Place the player.
    do {
        x = random_int_range(1, LEVEL_SIZE-1);
        y = random_int_range(1, LEVEL_SIZE-1);
    } while ((level[x + y * LEVEL_SIZE] != BIT(FLOOR)));
    level[x + y * LEVEL_SIZE] = BIT(FLOOR) | BIT(PLAYER);
}

// Similar to the scatter_placer, but uses flood fill to verify that
// completion critital entities are accessible by the player, and
// therefore the level is completable
// (assuming 'level' is well constructed).
void verified_scatter_placer(Level level, float * parameters) {
    float gold_chance = 0.07f;
    float enemy_chance = 0.03f;
    float spikes_chance = 0.03f;

    if (parameters) {
        gold_chance   = parameters[0];
        enemy_chance  = parameters[1];
        spikes_chance = parameters[2];
    }

    // Scatter some entities around.
    for (int y = 1; y < LEVEL_SIZE-1; ++y) {
        for (int x = 1; x < LEVEL_SIZE-1; ++x) {
            Tile t = level[x + y * LEVEL_SIZE];
            if ((t & BIT(WALL)) == 0) {
                if (chance(gold_chance))   t |= BIT(GOLD);   else
                if (chance(enemy_chance))  t |= BIT(ENEMY);  else
                if (chance(spikes_chance)) t |= BIT(SPIKES);
            }
            level[x + y * LEVEL_SIZE] = t;
        }
    }

    // For the critial entities (only one of each exist per level),
    // choose a random location, then verify it is acceptable.
    // If not, try again; repeat.
    int x, y;

    // Place the locked door on an empty floor tile.
    do {
        x = random_int_range(1, LEVEL_SIZE-1);
        y = random_int_range(1, LEVEL_SIZE-1);
    } while (level[x + y * LEVEL_SIZE] != BIT(FLOOR));
    level[x + y * LEVEL_SIZE] |= BIT(EXIT) | BIT(LOCK);

    Tile seen_entity_types = 0;

    // Place the key, verifying that some path exists between it and the exit.
    do {
        x = random_int_range(1, LEVEL_SIZE-1);
        y = random_int_range(1, LEVEL_SIZE-1);
        flood(level, x, y,
            BIT(FLOOR) | BIT(WALL) | BIT(SPIKES), BIT(FLOOR),
            flood_record_tiles, &seen_entity_types);
    } while ((level[x + y * LEVEL_SIZE] != BIT(FLOOR))
        && (seen_entity_types & BIT(EXIT)) == 0);
    level[x + y * LEVEL_SIZE] |= BIT(KEY);

    seen_entity_types = 0;

    // Place the player, verifying that some path exists between it,
    // the key, and the exit.
    do {
        x = random_int_range(1, LEVEL_SIZE-1);
        y = random_int_range(1, LEVEL_SIZE-1);
        flood(level, x, y,
            BIT(FLOOR) | BIT(WALL) | BIT(SPIKES), BIT(FLOOR),
            flood_record_tiles, &seen_entity_types);
    } while ((level[x + y * LEVEL_SIZE] != BIT(FLOOR))
        && (seen_entity_types & (BIT(EXIT) | BIT(KEY))) == 0);
    level[x + y * LEVEL_SIZE] |= BIT(PLAYER);
}

#ifndef NO_MAIN

int main(int argument_count, char ** arguments) {
    set_seed(1, 1);

    Level level = {0};

    empty_level(level);
    scatter_placer(level, NULL);

    reverse_verified_scatter_generator(level);

    write_level(stdout, level);
}

#endif
