#include <atari.h>
#include "benchmark.h"

#define SCREEN_SIZE_X 40
#define NO_ENEMIES 30
#define NO_ENTITIES (NO_ENEMIES+1)
#define PLAYER_INDEX (NO_ENEMIES)

#define ENTITY_DEAD 0
#define ENTITY_PLAYER 1
#define ENTITY_ENEMY 2
typedef unsigned char e_entity_type;

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

// ZP data

#pragma bss-name (push,"ZEROPAGE")
#pragma data-name (push,"ZEROPAGE")
unsigned char index1;
#pragma bss-name (pop)
#pragma data-name (pop)
#pragma zpsym ("index1");

/// Data
unsigned char *screen_ptr;
s_game_state game_state;

void place_enemy(unsigned char x, unsigned char y)
{
    game_state.entities.x[index1] = x;
    game_state.entities.y[index1] = y;
}

void set_entities()
{
    // set entities
    for (index1 = 0; index1 < NO_ENEMIES; index1++)
    {
        place_enemy((index1*5) % SCREEN_SIZE_X, index1 / 2 + 9);
        game_state.entities.hp[index1] = 99;
        game_state.entities.type[index1] = ENTITY_ENEMY;
    };
    // set player
    game_state.entities.hp[PLAYER_INDEX] = 99;
    game_state.entities.x[PLAYER_INDEX] = SCREEN_SIZE_X/2;
    game_state.entities.type[PLAYER_INDEX] = ENTITY_PLAYER;
};

char get_entity_tile(unsigned char type)
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


void draw_entity()
{
    const unsigned char FIRST_DIGIT_CHAR = 0x10;
    unsigned char *draw_ptr = &screen_ptr[game_state.entities.y[index1] * SCREEN_SIZE_X + game_state.entities.x[index1]];
    *draw_ptr = get_entity_tile(game_state.entities.type[index1]);
    *(++draw_ptr) = game_state.entities.hp[index1] / 10 + FIRST_DIGIT_CHAR;
    *(++draw_ptr) = game_state.entities.hp[index1] % 10 + FIRST_DIGIT_CHAR;
};

void damage_enemy()
{
    // damage
    if (game_state.entities.hp[index1] > 0)
        game_state.entities.hp[index1]--;
}

void one_frame()
{
    // draw entities
    for (index1 = 0; index1 < NO_ENEMIES; index1++)
    {
        damage_enemy();
        draw_entity();
    };
    // draw player
    index1 = PLAYER_INDEX;
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
