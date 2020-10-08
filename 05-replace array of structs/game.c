#include <atari.h>
#include "benchmark.h"

#define SCREEN_SIZE_X 40
#define NO_ENEMIES 30
#define NO_ENTITIES (NO_ENEMIES+1)
#define PLAYER_INDEX (NO_ENEMIES)

typedef enum e_entity_type {
    ENTITY_DEAD,
    ENTITY_PLAYER,
    ENTITY_ENEMY
} e_entity_type;

// +1 for player
typedef struct s_entity {
    unsigned char x[NO_ENTITIES];
    unsigned char y[NO_ENTITIES];
    unsigned char hp[NO_ENTITIES];
    e_entity_type type[NO_ENTITIES];
} s_entity;

typedef struct s_player {
    unsigned char attack;
} s_player;


typedef struct s_game_state {
    s_entity entities;
    s_player player;
} s_game_state;

/// Data
unsigned char *screen_ptr;
s_game_state game_state;

unsigned char place_enemy_index;
void place_enemy(unsigned char x, unsigned char y)
{
    game_state.entities.x[place_enemy_index] = x;
    game_state.entities.y[place_enemy_index] = y;
}

void set_entities()
{
    unsigned char index;
    // set entities
    for (index = 0; index < NO_ENEMIES; index++)
    {
        place_enemy_index = index;
        place_enemy((index*5) % SCREEN_SIZE_X, index / 2 + 9);
        game_state.entities.hp[index] = 99;
        game_state.entities.type[index] = ENTITY_ENEMY;
    };
    // set player
    game_state.entities.hp[PLAYER_INDEX] = 99;
    game_state.entities.x[PLAYER_INDEX] = SCREEN_SIZE_X/2;
    game_state.entities.type[PLAYER_INDEX] = ENTITY_PLAYER;
};

char get_entity_tile(e_entity_type type)
{
    switch(type)
    {
        case ENTITY_PLAYER:
            return 'p';
        case ENTITY_ENEMY:
            return 'e';
    }
    return 'x';
}


unsigned char draw_entity_index;
void draw_entity()
{
    const unsigned char FIRST_DIGIT_CHAR = 0x10;
    unsigned char *draw_ptr = &screen_ptr[game_state.entities.y[draw_entity_index] * SCREEN_SIZE_X + game_state.entities.x[draw_entity_index]];
    *draw_ptr = get_entity_tile(game_state.entities.type[draw_entity_index]);
    *(++draw_ptr) = game_state.entities.hp[draw_entity_index] / 10 + FIRST_DIGIT_CHAR;
    *(++draw_ptr) = game_state.entities.hp[draw_entity_index] % 10 + FIRST_DIGIT_CHAR;
};

unsigned char damage_enemy_index;
void damage_enemy()
{
    // damage
    if (game_state.entities.hp[damage_enemy_index] > 0)
        game_state.entities.hp[damage_enemy_index]--;
}

void one_frame()
{
    unsigned char index;

    // draw entities
    for (index = 0; index < NO_ENEMIES; index++)
    {
        damage_enemy_index = index;
        damage_enemy();

        draw_entity_index = index;
        draw_entity();
    };
    // draw player
    draw_entity_index = PLAYER_INDEX;
    draw_entity();

}

void main(void)
{
    unsigned char times;
    screen_ptr = OS.savmsc;
    set_entities();

    start_benchmark();
    for (times = 0; times < 100; ++times)
        one_frame();
    end_benchmark();

    for(;;);
}
