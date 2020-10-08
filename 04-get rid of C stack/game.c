#include <atari.h>
#include "benchmark.h"

#define SCREEN_SIZE_X 40
#define NO_ENEMIES 30
#define _countof(array) (sizeof(array) / sizeof(array[0]))

typedef enum e_entity_type {
    ENTITY_DEAD,
    ENTITY_PLAYER,
    ENTITY_ENEMY
} e_entity_type;

typedef struct s_entity {
    unsigned char x;
    unsigned char y;
    unsigned char hp;
    e_entity_type type;
} s_entity;

typedef struct s_player {
    s_entity entity;
    unsigned char attack;
} s_player;


typedef struct s_game_state {
    s_entity enemies[NO_ENEMIES];
    s_player player;
} s_game_state;

/// Data
unsigned char *screen_ptr;
s_game_state game_state;


s_entity *place_enemy_ptr;
void place_enemy(unsigned char x, unsigned char y)
{
    place_enemy_ptr->x = x;
    place_enemy_ptr->y = y;
}

void set_entities()
{
    unsigned char index;
    s_entity *e;
    // set enemies
    for (index = 0; index < _countof(game_state.enemies); index++)
    {
        e = &game_state.enemies[index];
        place_enemy_ptr = e;
        place_enemy((index*5) % SCREEN_SIZE_X, index / 2 + 9);
        e->hp = 99;
        e->type = ENTITY_ENEMY;
    };
    // set player
    game_state.player.entity.hp = 99;
    game_state.player.entity.x = SCREEN_SIZE_X/2;
    game_state.player.entity.type = ENTITY_PLAYER;
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


s_entity *draw_entity_ptr;
void draw_entity()
{
    const unsigned char FIRST_DIGIT_CHAR = 0x10;
    unsigned char *draw_ptr = &screen_ptr[draw_entity_ptr->y * SCREEN_SIZE_X + draw_entity_ptr->x];
    *draw_ptr = get_entity_tile(draw_entity_ptr->type);
    *(++draw_ptr) = draw_entity_ptr->hp / 10 + FIRST_DIGIT_CHAR;
    *(++draw_ptr) = draw_entity_ptr->hp % 10 + FIRST_DIGIT_CHAR;
};

s_entity *damage_enemy_ptr;
void damage_enemy()
{
    // damage
    if (damage_enemy_ptr->hp > 0)
        damage_enemy_ptr->hp--;
}

void one_frame()
{
    unsigned char index;
    s_entity *e;

    // draw enemies
    for (index=0;index < _countof(game_state.enemies);index++)
    {
        e = &game_state.enemies[index];
        damage_enemy_ptr = e;
        damage_enemy();

        draw_entity_ptr = e;
        draw_entity();
    };
    // draw player
    draw_entity_ptr = &game_state.player.entity;
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
