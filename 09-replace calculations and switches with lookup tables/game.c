#include <atari.h>
#include "benchmark.h"

#define SCREEN_SIZE_X 40
#define SCREEN_SIZE_Y 24
#define NO_ENEMIES 30
#define NO_ENTITIES (NO_ENEMIES+1)
#define PLAYER_INDEX (NO_ENEMIES)
#define MAX_LOOKUP_VALUE 100
#define FIRST_DIGIT_CHAR 0x10;

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
s_game_state game_state;


// Lookup tables
char get_entity_tile[] = {
    'x', 'p', 'e'
};

unsigned char div_10_lookup[MAX_LOOKUP_VALUE];
unsigned char mod_10_lookup[MAX_LOOKUP_VALUE];
unsigned char *screen_line_lookup[SCREEN_SIZE_Y];

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

void draw_entity()
{
    unsigned char *draw_ptr = screen_line_lookup[game_state.entities.y[index1]];
    draw_ptr += game_state.entities.x[index1];
    *draw_ptr = get_entity_tile[game_state.entities.type[index1]];
    *(++draw_ptr) = div_10_lookup[game_state.entities.hp[index1]];
    *(++draw_ptr) = mod_10_lookup[game_state.entities.hp[index1]];
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

void init_lookup_tables()
{
    unsigned char *screen_ptr = OS.savmsc;
    // init screen lookup
    for (index1 = 0; index1 < SCREEN_SIZE_Y; ++index1)
        screen_line_lookup[index1] = &screen_ptr[index1 * SCREEN_SIZE_X];

    for (index1 = 0; index1 < MAX_LOOKUP_VALUE; ++index1)
    {
        div_10_lookup[index1] = index1/10 + FIRST_DIGIT_CHAR;
        mod_10_lookup[index1] = index1%10 + FIRST_DIGIT_CHAR;
    }
}


void main(void)
{
    unsigned char times;

    init_lookup_tables();
    set_entities();

    start_benchmark();

    for (times = 0; times < 100; ++times)
        one_frame();
    end_benchmark();

    for(;;);
}
