# Advanced optimizations in CC65

CC65 is a mature cross-compiler of C language for 6502 processor. Some people tried to use it (e.g. [here](https://www.xtof.info/blog/?p=714), [here](https://www.lemon64.com/forum/viewtopic.php?t=70488), or [here](https://atariage.com/forums/topic/261463-c-or-assembler-for-game-development/)) and got discouraged by the quality of the generated code, often not understanding why the code generated was slow and big. This article is aiming to show that with a few changes in the coding style you can achieve both speed and size comparable to assembly level, still having majority of the benefits of working in higher-level language.  This article base on the CC65 version 2.18 (April 2020) and we may expect that in the future the compiler will handle more optimizations mentioned here automatically.

# Table of contents
- [Why CC65?](#Why-CC65)
- [CC65 alternatives](#CC65-alternatives)
- [Sample program](#Sample-program)
  * [Compilation](#Compilation)
- [Lets start optimizations](#Lets-start-optimizations)
  * [Optimization basics](#Optimization-basics)
  * [01 - Start - no optimizations - 528 ticks](##1---start---no-optimizations---528-ticks)
  * [02 - Compiler Options - 392 ticks ](#02---compiler-options---392-ticks-34-speedup-from-the-previous-state)
  * [03 - Smallest possible unsigned data types - 380 ticks](#03---smallest-possible-unsigned-data-types---380-ticks-3-speedup-often-more-with-bigger-code)
  * [04 - Get rid of C stack, globals are your friend - 334 ticks](#04---get-rid-of-c-stack-globals-are-your-friend---334-ticks-14-speedup)
  * [05 - Replace "array of structs" to "struct of arrays" - 305 ticks](#05---replace-array-of-structs-to-struct-of-arrays---305-ticks-10-speedup-often-more-with-bigger-code)
  * [06 - Get rid of enums - 296 ticks](#06---get-rid-of-enums---296-ticks-3-speedup)
  * [07 - Place commonly used variables on Zero Page - still 296 ticks](#07---place-commonly-used-variables-on-zero-page---still-296-ticks-no-speedup-here-but-makes-real-difference-if-big-or-nested-loops-are-used)
  * [08 - Get rid of parameter passing - 296 ticks](#08---get-rid-of-parameter-passing---296-ticks-no-speedup-here-but-works-better-together-further-optimizations)
  * [09 - Replace calculations, switches and screen access by Lookup Tables - 67 ticks](#09---replace-calculations-switches-and-screen-access-by-lookup-tables---67-ticks-342-speedup)
  * [10 - Handle "integer promotion" cases and improve array access - 34 ticks](#10---handle-integer-promotion-cases-and-improve-array-access---34-ticks-97-speedup)
  * [11 - Improve array accesses even further - 32 ticks](#11---improve-array-accesses-even-further---32-ticks-6-speedup)
  * [12 - Inline functions, activate additional "register" keyword optimizations - 29 ticks](#12---inline-functions-activate-additional-register-keyword-optimizations---29-ticks-6-speedup)
  * [Identify code critical places and rewrite them in assembly](#identify-code-critical-places-and-rewrite-them-in-assembly)
  * [What else to optimize was not covered in this tutorial?](#what-else-to-optimize-was-not-covered-in-this-tutorial)
    + [Dynamic memory allocations](#dynamic-memory-allocations)
    + [Multidimensional arrays](#multidimensional-arrays)
    + [Default runtime functions vs specialized functions](#default-runtime-functions-vs-specialized-functions)
    + [Cache and precalc](#cache-and-precalc)
    + [Increasing and decreasing variables](#increasing-and-decreasing-variables)
    + [Drawing single pixels on the screen and interrupt handlers](#drawing-single-pixels-on-the-screen-and-interrupt-handlers)
- [Summary](#Summary)

## Why CC65?

CC65 is one of the most "C standard compliant " environments for 6502. It has close [compliance with C99 standard](https://cc65.github.io/doc/cc65.html#s4), which is great for any C/C++ programmers and a big advantage for people with knowledge of programming languages that base on C syntax.

Pros:

* Writing C code in CC65 you can compile for both 6502 and other platforms. Majority of my projects were compiled in parallel in Visual Studio or GCC, and while all the game logic was equal, only the "presentation layer" was different and platform specific. This allows to implement complex game logic and debug it using modern IDE.
* While assembler wins in performance and code size, writing C code is incredibly fast in comparison to asm. From my experience it takes at least 5 times less time than equivalent code in assembler and later is much easier to maintain.
* It integrates well with assembly and code-critical parts you can write in asm. Rule 80/20 applies often and just writing speed-critical parts in assembly you can keep majority of the code in C.
* There are different benchmarks available comparing CC65 with other languages (e.g. [this one](https://github.com/KarolS/millfork-benchmarks/tree/master/6502)) that show how bad this compiler is. Usually the C code written in them is of poor quality,  intentionally or not written to highlight strengths of other compiler.  [I was able to rewrite](https://github.com/KarolS/millfork-benchmarks/issues/2) (or [here](https://atariage.com/forums/topic/240919-mad-pascal/?do=findComment&comment=3395999)) majority of such benchmarks in CC65 to [improve performance](https://github.com/KarolS/millfork-benchmarks/issues/2) by hundreds of percent.
* Recently there was [a big comparison](https://atariage.com/forums/topic/240919-mad-pascal/?do=findComment&comment=4471049) of different languages prepared by Zbyti and CC65 in many tests was among the top performers, where the output code was only 20-40% slower than hand-optimized assembler code.
* CC65 is very popular and many great games for [Atari](https://atariage.com/forums/topic/259931-atari-8-bit-games-in-c-cc65/), [NES](https://shiru.untergrund.net/software.shtml) or [C64](https://github.com/cc65/wiki/wiki/Applications-written-in-C-or-C-with-assembler) were created using this environment.

- CC65 is in [active development on GitHub](https://cc65.github.io/) and as for April 2020 with more than [65 contributors](https://github.com/cc65/cc65/graphs/contributors).

Cons:

* The compiler is not perfect. While it can generate good code, comparable to other best compilers on 6502, it still generates code that easily could be optimized. I hope to find more time one day to get into the compiler code to improve the main optimization bottlenecks.
* As stated in the documentation, it does no high-level optimizations. It means that many constructs need to be simplified by the programmer to achieve good performance. This article is showing many of such best practices.
* Working with linker configuration to achieve the best memory utilization has steep learning curve, but fortunately for the beginners it can be skipped until needed.

Knowledge of writing efficient code in C is abandoned nowadays due to modern compilers that do so good optimizations, that often it is hard to manually write as good assembly. In some cases they even use superoptimizers that assure optimal code. However if you read books about the game programming from 90s, there is a lot of information what tricks were used to achieve the best performance without switching to assembly. You can still [find these "old tricks"](http://icps.u-strasbg.fr/~bastoul/local_copies/lee.html) in the Internet, however majority of them do not work on 6502 or makes even opposite effect due to 6502 CPU architecture! I'm aware of only [one guide](https://github.com/Fabrizio-Caruso/8bitC/blob/master/8bitC_ENG.md) of hints for coding in C for 6502 and I hope this article will fill the gap.

## CC65 alternatives
Great article that every programmer for 6502 should read is [David A. Wheeler's "6502 Language Implementation Approaches"](https://dwheeler.com/6502/). It explains very well difficulties of making a good compiler for this platform. 

Here is a list of many - but not all - CC65 alternatives:

- Assembly - unquestionably wins in terms of code performance and code size, however coding in asm is very time consuming in comparison to higher-level languages and "maintability" of the code is poor, especially if you return to it after some time, or for a team-work.
- Mad Pascal - new language compatible with subset of FreePascal, primarily addressing Atari platform. In active development by one author. Has performance comparable to CC65 but more Atari-specific external libraries.
- Millfork - in early stage of development, new language that includes in the language 6502 specific features for generation of very effective code. Primarily for C64 but with growing support of other platforms
- KickC - in early stage of development. Currently as subset of C language aiming to generate very efficient code. Author is actively working on closer compatibility with C standard.
- Plasma - language that is often closer to assembler than higher-level languages. Aiming to provide a lot of control over generated code. Recently updated.
- Action! - old language for high-performance code, no cross-compiler available, some language limitations prevent writing bigger programs.
- Effectus - new cross-compiler for Action language, one-man project, that was suspended for a while and recently got resurrected with new "Mad Pascal backend".
- Atalan - one-man project. Atalan unfortunately ended its life.
- GCC-6502 - there are a few attempts to use marvelous GCC compiler to produce 6502 code, usually by placing registers on Zero Page. None is in active develpement.
- PyMite - variant of Python for 6502, abandoned.
- Some other 6502 languages are described [here](http://8bitworkshop.com/blog/compilers/higher-level-6502-programming.md.html)

Languages dedicated to 6502 (or other 8bit processors) usually are use subset of "big languages", therefore why not to just use fast subset of C, keeping as an option full language compatibility when needed?

## Sample program

High-performance code is usually connected with games and I will use such example in this article.

Standard elements of a game are:

- Game state (game data)
- Game logic (game code)
- Representation of the game state (on screen)
- Input handling

In this article I'm not going to focus on "input handling" but will focus on the "game state", "game logic" and "representation". In majority of games there is one player and multiple enemies, both described by internal state like position on the screen or hit points. Representation on the screen in 6502 games for performance and memory reasons is often done using available "character modes" and here I will use similar approach.

The full code (excluding "benchmark.h" responsible for displaying execution time) is below. Writing it I tried to follow [one of the C coding standards](https://www.doc.ic.ac.uk/lab/cplus/cstyle.html) and some "good practices" for code clarity.  The code is already not following some of the basic recommendations of how to write an effective code in CC65 documented [here](https://cc65.github.io/doc/coding.html), but is trying to represent how C programmer of "modern platforms" [would approach](https://atariage.com/forums/topic/185581-atari-programming-in-cc65/?do=findComment&comment=2822822) the problem.

```c
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
    int x;
    int y;
    int hp;
    e_entity_type type;
} s_entity;

typedef struct s_player {
    s_entity entity;
    int attack;
} s_player;


typedef struct s_game_state {
    s_entity enemies[NO_ENEMIES];
    s_player player;
} s_game_state;

void place_enemy(s_entity *e_ptr, int x, int y)
{
    e_ptr->x = x;
    e_ptr->y = y;
}

void set_entities(s_game_state *game_state)
{
    int index;
    s_entity *e;
    // set enemies
    for (index=0;index<_countof(game_state->enemies);index++)
    {
        e = &game_state->enemies[index];
        place_enemy( e, (index*5) % SCREEN_SIZE_X, index / 2 + 9);
        e->hp = 99;
        e->type = ENTITY_ENEMY;
    };
    // set player
    game_state->player.entity.hp = 99;
    game_state->player.entity.x = SCREEN_SIZE_X/2;
    game_state->player.entity.type = ENTITY_PLAYER;
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

void draw_entity(unsigned char *screen_ptr, s_entity *e_ptr)
{
    const int FIRST_DIGIT_CHAR = 0x10;
    unsigned char *draw_ptr = &screen_ptr[e_ptr->y * SCREEN_SIZE_X + e_ptr->x];
    *draw_ptr = get_entity_tile(e_ptr->type);
    *(++draw_ptr) = e_ptr->hp / 10 + FIRST_DIGIT_CHAR;
    *(++draw_ptr)   = e_ptr->hp % 10 + FIRST_DIGIT_CHAR;
};

void damage_enemy(s_entity *e_ptr)
{
    // damage         
    if (e_ptr->hp > 0)
        e_ptr->hp--;
}

void one_frame(s_game_state *game_state, unsigned char *screen_ptr)
{
    int index;
    s_entity *e;
    
    // draw enemies
    for (index=0;index < _countof(game_state->enemies);index++)
    {
        e = &game_state->enemies[index];
        damage_enemy(e);
        draw_entity(screen_ptr, e);
    };
    // draw player
    draw_entity(screen_ptr, &game_state->player.entity);
    
}

void main(void)
{
    unsigned char *screen_ptr;
    unsigned int times;
    
    s_game_state game_state;

    screen_ptr = OS.savmsc;
    set_entities(&game_state);

    start_benchmark();    
    for (times=0;times<100;++times)
        one_frame(&game_state, screen_ptr);
    end_benchmark();
    
    for(;;);
}
```

the "representation" of the game state is visible on this screenshot:

![screenshot](/01-start/screenshot.png)

Player is represented by letter 'p' and enemies by letters 'e'. To the right of these entities there are two digits that display their HitPoints. The font is the standard system one as we did not change the graphics anyhow. Lets imagine that player is a wizard that casted "freeze spell" and now all the enemies are frozen and their HitPoints are going towards zero.  After 100 iterations the game ends with all the enemies frozen.

### Compilation

To compile the program we can use the following command line:

```bash
cl65 -t atari -o game.xex game.c
```

however it is useful to add additional compilation options to generate additional output files

```bash
cl65 -t atari -Ln game.lbl --listing game.lst --add-source -o game.xex game.c
```

* game.lbl is VICE label file that can be loaded by Altirra emulator and later used by Performance Monitor profiler
* game.lst is output assembly listing that with --add-source option will include related C code parts as comments

# Lets start optimizations

## Optimization basics

There are a few language-independent or compiler-independent rules of optimization:

* Do not start with optimizations. Optimize at the end, because
  * optimized code is hard to read
  * optimized code is slow to write
  * optimized code is bug-prone
  * when you remove optimized code due to design changes you remove all the invested time
* Use early proper data structures and algorithms. Good algorithm can bring much higher performance boost than low-level optimizations.
* In case of 6502 platform, think in advance about memory layout how to make access to data fast, without additional computations.

The following "best practices" are going from *basic ones* to *extreme ones* and it is up to you which make sense to apply for your code (profile it!) in price of code readability.

## 01 - Start - no optimizations - 528 ticks

When compiled with the command line as above, execution ends with information that the simulation took **528 ticks**. Each tick is increased on PAL system each 1/50 second by the OS. Lets find out how well we can optimize the code from the initial state (528 ticks).

Looking at the output code game.lst file, anyone who knows at least basics of 6502 would probably immediately delete the file with statement "this is useless".  Lets take a look at very simple function:

```c
void damage_enemy(s_entity *e_ptr)
{
    // damage         
    if (e_ptr->hp > 0)
        e_ptr->hp--;
}
```

which is getting compiled into:

```assembly
000234r 1               ; --------------------------------------------------------------
000234r 1               ; void __near__ damage_enemy (__near__ struct s_entity *)
000234r 1               ; --------------------------------------------------------------
000234r 1               
000234r 1               .segment    "CODE"
000234r 1               
000234r 1               .proc    _damage_enemy: near
000234r 1               
000234r 1               .segment    "CODE"
000234r 1               
000234r 1               ;
000234r 1               ; {
000234r 1               ;
000234r 1  20 rr rr         jsr     pushax
000237r 1               ;
000237r 1               ; if (e_ptr->hp > 0)
000237r 1               ;
000237r 1  A0 01            ldy     #$01
000239r 1  20 rr rr         jsr     ldaxysp
00023Cr 1  A0 05            ldy     #$05
00023Er 1  20 rr rr         jsr     ldaxidx
000241r 1  C9 01            cmp     #$01
000243r 1  8A               txa
000244r 1  E9 00            sbc     #$00
000246r 1  70 02            bvs     L0086
000248r 1  49 80            eor     #$80
00024Ar 1  0A           L0086:    asl     a
00024Br 1  A9 00            lda     #$00
00024Dr 1  A2 00            ldx     #$00
00024Fr 1  2A               rol     a
000250r 1  D0 03 4C rr      jeq     L0084
000254r 1  rr           
000255r 1               ;
000255r 1               ; e_ptr->hp--;
000255r 1               ;
000255r 1  A0 01            ldy     #$01
000257r 1  20 rr rr         jsr     ldaxysp
00025Ar 1  20 rr rr         jsr     pushax
00025Dr 1  A0 05            ldy     #$05
00025Fr 1  20 rr rr         jsr     ldaxidx
000262r 1  85 rr            sta     regsave
000264r 1  86 rr            stx     regsave+1
000266r 1  20 rr rr         jsr     decax1
000269r 1  A0 04            ldy     #$04
00026Br 1  20 rr rr         jsr     staxspidx
00026Er 1  A5 rr            lda     regsave
000270r 1  A6 rr            ldx     regsave+1
000272r 1               ;
000272r 1               ; }
000272r 1               ;
000272r 1  20 rr rr     L0084:    jsr     incsp2
000275r 1  60               rts
```

it is terrible, indeed.

## 02 - Compiler Options - 392 ticks (34% speedup from the previous state)

CC65 does not turn on optimizations by default. The two main ones to turn on are:

* -Osir (which is equal to "-O -Os - Oi -Or") - it enables optimization of code, inlines known functions and enables use of "registry keyword".
* -Cl or --static-locals - this makes "static" local variables in function instead of putting them on stack. Function parameters with this are still passed through the stack.

The final command line to compile looks as follows:

```bash
cl65 -t atari  -Ln game.lbl -Osir -Cl --listing game.lst --add-source -o game.xex game.c
```

The stack on 6502 is very small and CC65 implemented very slow "software stack". With making all local variables static, they are placed in hard-coded locations in data section and access to them is much faster.

On 6502 not only stack is slow. Any level of indirection in access to data is slow. When variables can be compiled to known "static locations", CPU can access them very fast.

After turning on optimizations the "damage_enemy" function looks better (majority of calls to runtime is out) but still very poorly:

```assembly
000274r 1               ; --------------------------------------------------------------
000274r 1               ; void __near__ damage_enemy (__near__ struct s_entity *)
000274r 1               ; --------------------------------------------------------------
000274r 1               
000274r 1               .segment    "CODE"
000274r 1               
000274r 1               .proc    _damage_enemy: near
000274r 1               
000274r 1               .segment    "CODE"
000274r 1               
000274r 1               ;
000274r 1               ; {
000274r 1               ;
000274r 1  20 rr rr         jsr     pushax
000277r 1               ;
000277r 1               ; if (e_ptr->hp > 0)
000277r 1               ;
000277r 1  A0 01            ldy     #$01
000279r 1  B1 rr            lda     (sp),y
00027Br 1  85 rr            sta     ptr1+1
00027Dr 1  88               dey
00027Er 1  B1 rr            lda     (sp),y
000280r 1  85 rr            sta     ptr1
000282r 1  A0 05            ldy     #$05
000284r 1  B1 rr            lda     (ptr1),y
000286r 1  AA               tax
000287r 1  88               dey
000288r 1  B1 rr            lda     (ptr1),y
00028Ar 1  C9 01            cmp     #$01
00028Cr 1  8A               txa
00028Dr 1  E9 00            sbc     #$00
00028Fr 1  70 02            bvs     L008E
000291r 1  49 80            eor     #$80
000293r 1  10 20        L008E:    bpl     L008C
000295r 1               ;
000295r 1               ; e_ptr->hp--;
000295r 1               ;
000295r 1  A0 01            ldy     #$01
000297r 1  B1 rr            lda     (sp),y
000299r 1  AA               tax
00029Ar 1  88               dey
00029Br 1  B1 rr            lda     (sp),y
00029Dr 1  20 rr rr         jsr     pushax
0002A0r 1  85 rr            sta     ptr1
0002A2r 1  86 rr            stx     ptr1+1
0002A4r 1  A0 05            ldy     #$05
0002A6r 1  B1 rr            lda     (ptr1),y
0002A8r 1  AA               tax
0002A9r 1  88               dey
0002AAr 1  B1 rr            lda     (ptr1),y
0002ACr 1  38               sec
0002ADr 1  E9 01            sbc     #$01
0002AFr 1  B0 01            bcs     L00CD
0002B1r 1  CA               dex
0002B2r 1  20 rr rr     L00CD:    jsr     staxspidx
0002B5r 1               ;
0002B5r 1               ; }
0002B5r 1              ;
0002B5r 1  4C rr rr     L008C:    jmp     incsp2
```

## 03 - Smallest possible unsigned data types - 380 ticks (3% speedup, often more with bigger code)

6502 is 8bit CPU and in our game code "int" data type was used. By C language definition "int" should be the fasted data type on the platform, however standard C89 of the language was defined when there were already 16bit CPU and it set "int" as minimum 16bits for majority of compilers., including CC65. 

Here we replaced this:

```C
typedef struct s_entity {
    int x;
    int y;
    int hp;
    e_entity_type type;
} s_entity;

typedef struct s_player {
    s_entity entity;
    int attack;
} s_player;
```

with:

```c
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
```

On 6502 the fastest data type is "unsigned char" and after replacing all the "ints" with "unsigned char" we went down to 380 ticks. Not a big improvement, but very important one as it saves both CPU time and space. Especially in case of more complex code the gain is big.

## 04 - Get rid of C stack, globals are your friend - 334 ticks (14% speedup)

As mentioned earlier, C stack on 6502 is slow and generates bloated code. We already have "static locals" but the stack is still used to pass parameters to the functions. Next step is moving the function parameters to global space. Then we do not need to use software stack anymore. Access to such variables is fast, because now the former stack function parameter is in predefined location.

```c
s_entity *damage_enemy_ptr;
void damage_enemy()
{
    // damage         
    if (damage_enemy_ptr->hp > 0)
        damage_enemy_ptr->hp--;
}
```

Disadvantage is that such coding greatly lowers readability of the code and therefore this optimization I recommend to use as one of the lasts. Remember ["premature optimization is the root of all evil"](https://en.wikipedia.org/wiki/Program_optimization#When_to_optimize). 

## 05 - Replace "array of structs" to "struct of arrays" - 305 ticks (10% speedup, often more with bigger code)

"Array of structs" is standard way of representing objects when writing in any modern higher-level language. It allows to address objects by pointer and allows easy access to object fields by increasing the pointer by object size. 

```c
typedef struct s_entity {
    unsigned char x;
    unsigned char y;
    unsigned char hp;
    e_entity_type type;
} s_entity;

// Array of structs
s_entity enemies[NO_ENEMIES];
```

Unfortunately 6502 does not work well with pointers - it is 8bit processor but to address available memory space you need 16bit. To allow nested structs and complex pointer arithmetics the compiler cannot do strong optimizations here which leads to ineffective and bloated code.
No matter how good compiler is, 16bit pointers are ineffective on 6502. Preferably they should be changed into array indexes (of "unsigned char" type), and arrays then should be max. 256 bytes long.

What we can do, and I recommend to start programming with such approach, is to change "array of structs" to "struct of arrays":

```C
typedef struct s_entity {
    unsigned char x[NO_ENEMIES];
    unsigned char y[NO_ENEMIES];
    unsigned char hp[NO_ENEMIES];
    e_entity_type type[NO_ENEMIES];
} s_entity;

s_entity enemies;
```

With such approach you have to address the fields differently, but in very similar way:

```C
// instead of 
enemies[enemy_index].hp = value;
// you use
enemies.hp[enemy_index] = value
```

It also disallows to use pointers to specific objects, and you are forced to use indexes. To make it even more effective on 6502, keep the arrays below 256 so you can index them by "unsigned char". 

In our case it improved performance by about 10%, but in bigger projects the gain is usually much higher, especially on the size of the generated code.

Our simple function "damage_enemy" is getting smaller with this approach, but still there is place for improvement:

```c
00018Fr 1               ; --------------------------------------------------------------
00018Fr 1               ; void __near__ damage_enemy (void)
00018Fr 1               ; --------------------------------------------------------------
00018Fr 1               
00018Fr 1               .segment    "CODE"
00018Fr 1               
00018Fr 1               .proc    _damage_enemy: near
00018Fr 1               
00018Fr 1               .segment    "CODE"
00018Fr 1               
00018Fr 1               ;
00018Fr 1               ; if (game_state.entities.hp[damage_enemy_index] > 0)
00018Fr 1               ;
00018Fr 1  AC rr rr         ldy     _damage_enemy_index
000192r 1  B9 rr rr         lda     _game_state+62,y
000195r 1  F0 1C            beq     L00A0
000197r 1               ;
000197r 1               ; game_state.entities.hp[damage_enemy_index]--;
000197r 1               ;
000197r 1  A9 rr            lda     #<(_game_state+62)
000199r 1  A2 rr            ldx     #>(_game_state+62)
00019Br 1  18               clc
00019Cr 1  6D rr rr         adc     _damage_enemy_index
00019Fr 1  90 01            bcc     L00A6
0001A1r 1  E8               inx
0001A2r 1  20 rr rr     L00A6:    jsr     pushax
0001A5r 1  85 rr            sta     ptr1
0001A7r 1  86 rr            stx     ptr1+1
0001A9r 1  A0 00            ldy     #$00
0001ABr 1  B1 rr            lda     (ptr1),y
0001ADr 1  38               sec
0001AEr 1  E9 01            sbc     #$01
0001B0r 1  4C rr rr         jmp     staspidx
0001B3r 1               ;
0001B3r 1               ; }
0001B3r 1               ;
0001B3r 1  60           L00A0:    rts
```

## 06 - Get rid of enums - 296 ticks (3% speedup)

In CC65 sizeof(enum_type) is 2 (equals int) and this is not the fastest data type on 6502. Change enums to "unsigned char" and enum values to constant #DEFINE. On 6502 use of constant values is faster than use of variables as it does not require reading values from memory. 

We are replacing:

```c
typedef enum e_entity_type {
    ENTITY_DEAD,
    ENTITY_PLAYER,
    ENTITY_ENEMY
} e_entity_type;
```

with:

```c
#define ENTITY_DEAD 0
#define ENTITY_PLAYER 1
#define ENTITY_ENEMY 2
typedef unsigned char e_entity_type;
```

## 07 - Place commonly used variables on Zero Page - still 296 ticks (no speedup here, but makes real difference if big or nested loops are used)

On 6502 first 256 bytes of memory (Zero Page) is a special place to which CPU has much faster access. It's therefore beneficial to locate often used variables or pointers there, however the place is sparse and often used also by external libraries you may use in your software (like music player).

CC65 allows to place variables on zero page in 3 different ways:

1. using "register" keyword - this [documentation](https://cc65.github.io/doc/cc65.html#register-vars) is worth reading
   * pros: safe and easy to use, compiler has some special optimizations done for them
   * cons: only a few available (6 by default), with each declaration the previous content is placed on stack (unfortunately CC65 does not use "static locals" for them even if defined!)
2. In linked assembly code in .SEGMENT "ZEROPAGE" 
3. From C language by using [#pragma bss-name](https://cc65.github.io/doc/cc65.html#ss7.2) and [#pragma data-name](https://cc65.github.io/doc/cc65.html#ss7.7)

We will use in our code the third method and will put them in the standard "ZEROPAGE" segment defined in the default linker configuration file:

```c
// ZP data
#pragma bss-name (push,"ZEROPAGE")
#pragma data-name (push,"ZEROPAGE")
unsigned char index1;
#pragma bss-name (pop)
#pragma data-name (pop)
#pragma zpsym ("index1"); 
```

By default the linker configuration for Atari has the Zero Page available for the programmer defined as follows:

```php
ZP:         file = "", define = yes, start = $0082, size = $007E;
```

which gives 126 bytes space, which is often more than enough for the program.

## 08 - Get rid of parameter passing - 296 ticks (no speedup here, but works better together further optimizations)

One optimization that often greatly improves performance is getting rid of parameter passing at all in price of code readability (therefore should be used as the last resort optimization).

With moving parameters away from stack, currently we do:

```c
void one_frame()
{
    // draw entities
    for (index1=0;index1 < NO_ENEMIES;index1++)
    {
        damage_enemy_index = index1;
        damage_enemy();
        
        draw_entity_index = index1;
        draw_entity();         
    };
    // draw player
    draw_entity_index = PLAYER_INDEX;
    draw_entity();
}
```

However with global variables we can use internally in damage_enemy() and draw_entity() global variable index1 instead of using extra parameters. This will not only make code less readable but also will disallow to modify the parameters inside the functions, therefore requires extra caution when using it.

After modification the functions look like this:

```c
// index1 is used as parameter - global Zero Page variable
void damage_enemy()
{
    // damage         
    if (game_state.entities.hp[index1] > 0)
        game_state.entities.hp[index1]--;
}

void one_frame()
{
    // draw entities
    for (index1=0;index1 < NO_ENEMIES;index1++)
    {
        damage_enemy();        
        draw_entity();         
    };
    // draw player
    index1 = PLAYER_INDEX;
    draw_entity();    
}
```

## 09 - Replace calculations, switches and screen access by Lookup Tables - 67 ticks (342% speedup)

Now we are getting to optimization that is often mandatory for any 6502 code, no matter if higher-level language or assembler is used. 6502 has limited instruction set and does not have very useful instructions like multiplication or division. To perform such operations it requires to run dedicated multiplication or division nested loops of code, or use some arithmetic tables. It is getting even more costly if multiplication or division is applied to data types larger than 1 bytes, like integers.

For the best performance whenever we have operations that multiply or divide variables by constant value (e.g. by 10 or by screen size), we should replace them by [Lookup Tables](https://en.wikipedia.org/wiki/Lookup_table). Such Lookup Tables often in assembler can be initialized by macros, however in case of C they need to be calculated using external tool and included as arrays (bin2c tool), or pre-calculated using internal code.

Similarly for fast access of screen data we can precalculate pointers to screen lines:

```c
#define MAX_LOOKUP_VALUE 100
#define SCREEN_SIZE_Y 24
unsigned char div_10_lookup[MAX_LOOKUP_VALUE];
unsigned char mod_10_lookup[MAX_LOOKUP_VALUE];
unsigned char *screen_line_lookup[SCREEN_SIZE_Y];

void init_lookup_tables()
{
    unsigned char *screen_ptr = OS.savmsc;
    // init screen lookup
    for (index1 = 0;index1<SCREEN_SIZE_Y;++index1)
        screen_line_lookup[index1] = &screen_ptr[index1 * SCREEN_SIZE_X];

    for (index1 = 0;index1<MAX_LOOKUP_VALUE;++index1)
    {
        div_10_lookup[index1] = index1/10 + FIRST_DIGIT_CHAR;
        mod_10_lookup[index1] = index1%10 + FIRST_DIGIT_CHAR;
    }    
}
```

then:

```c
// instead of using division:
result = value / 10;
// we use
result = div_10_lookup[value];
```

Similarly if a "switch" statement has role of a "data converter" (common cases are key codes, screen codes or rotation/reverting of object direction) it can be replaced by lookup table:

```c
#define ENTITY_DEAD 0
#define ENTITY_PLAYER 1
#define ENTITY_ENEMY 2

// this function
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

// can be replaced by Lookup Table
char get_entity_tile[] = {
    'x', 'p', 'e'
};
```

## 10 - Handle "integer promotion" cases and improve array access - 34 ticks (97% speedup)

C language does ["integer promotion"](https://www.geeksforgeeks.org/integer-promotions-in-c/), for performing math or logical operations, because "int" should be the fastest data type. On 6502 the fastest is "unsigned char", therefore such promotion in CC65 lowers performance.  While compiler generates code operating on 16bit values, later the code gets optimized to 8bit values by optimizer. Usually it works, however CC65 has problems with some constructs:

* when complex calculation is performed in one line
* result of calculation in one line is copied to array field or pointer

* complex calculation is performed in "if" statement

To help the compiler you can use intermediate variable before assignment. The best if such intermediate variable is put on the Zero Page for performance.

In the generated assembly listing you can usually find places that can be optimized by looking for "(ptr1),y" string. It often means that compiler wasn't able to make simple assignment and is doing write by calculated pointer address.

Take a look at the following function:

```c
void damage_enemy()
{
    // damage         
    if (game_state.entities.hp[index1] > 0)
        game_state.entities.hp[index1]--;
}
```

it is getting compiled into poor code:

```assembly
000133r 1               ; --------------------------------------------------------------
000133r 1               ; void __near__ damage_enemy (void)
000133r 1               ; --------------------------------------------------------------
000133r 1               
000133r 1               .segment    "CODE"
000133r 1               
000133r 1               .proc    _damage_enemy: near
000133r 1               
000133r 1               .segment    "CODE"
000133r 1               
000133r 1               ;
000133r 1               ; if (game_state.entities.hp[index1] > 0)
000133r 1               ;
000133r 1  A4 rr            ldy     _index1
000135r 1  B9 rr rr         lda     _game_state+62,y
000138r 1  F0 1B            beq     L00A1
00013Ar 1               ;
00013Ar 1               ; game_state.entities.hp[index1]--;
00013Ar 1               ;
00013Ar 1  A9 rr            lda     #<(_game_state+62)
00013Cr 1  A2 rr            ldx     #>(_game_state+62)
00013Er 1  18               clc
00013Fr 1  65 rr            adc     _index1
000141r 1  90 01            bcc     L00A7
000143r 1  E8               inx
000144r 1  20 rr rr     L00A7:    jsr     pushax
000147r 1  85 rr            sta     ptr1
000149r 1  86 rr            stx     ptr1+1
00014Br 1  A0 00            ldy     #$00
00014Dr 1  B1 rr            lda     (ptr1),y
00014Fr 1  38               sec
000150r 1  E9 01            sbc     #$01
000152r 1  4C rr rr         jmp     staspidx
000155r 1               ;
000155r 1               ; }
000155r 1               ;
000155r 1  60           L00A1:    rts
000156r 1               
000156r 1               .endproc
```

However after adding intermediate variable:

```C
void damage_enemy()
{
    // damage     
    calc1 = game_state.entities.hp[index1];
    if (calc1 > 0)
    {
        --calc1;
        game_state.entities.hp[index1] = calc1;
    }
}
```

we have very nice code generated: 

```assembly
00013Cr 1               ; --------------------------------------------------------------
00013Cr 1               ; void __near__ damage_enemy (void)
00013Cr 1               ; --------------------------------------------------------------
00013Cr 1               
00013Cr 1               .segment    "CODE"
00013Cr 1               
00013Cr 1               .proc    _damage_enemy: near
00013Cr 1               
00013Cr 1               .segment    "CODE"
00013Cr 1               
00013Cr 1               ;
00013Cr 1               ; calc1 = game_state.entities.hp[index1];
00013Cr 1               ;
00013Cr 1  A4 rr            ldy     _index1
00013Er 1  B9 rr rr         lda     _game_state+62,y
000141r 1  85 rr            sta     _calc1
000143r 1               ;
000143r 1               ; if (calc1 > 0)
000143r 1               ;
000143r 1  A5 rr            lda     _calc1
000145r 1  F0 09            beq     L00AC
000147r 1               ;
000147r 1               ; --calc1;
000147r 1               ;
000147r 1  C6 rr            dec     _calc1
000149r 1               ;
000149r 1               ; game_state.entities.hp[index1] = calc1;
000149r 1               ;
000149r 1  A4 rr            ldy     _index1
00014Br 1  A5 rr            lda     _calc1
00014Dr 1  99 rr rr         sta     _game_state+62,y
000150r 1               ;
000150r 1               ; }
000150r 1               ;
000150r 1  60           L00AC:    rts
000151r 1               
000151r 1               .endproc
```

## 11 - Improve array accesses even further - 32 ticks (6% speedup)

Looking at the function above, did we achieve the optimal code generation by the compiler? Unfortunately, not. One time critical function that still requires improvement is "draw_entity", which still looks bad:

```assembly
0000ABr 1               ; ---------------------------------------------------------------
0000ABr 1               ; void __near__ draw_entity (void)
0000ABr 1               ; ---------------------------------------------------------------
0000ABr 1               
0000ABr 1               .segment    "CODE"
0000ABr 1               
0000ABr 1               .proc    _draw_entity: near
0000ABr 1               
0000ABr 1               .segment    "CODE"
0000ABr 1               
0000ABr 1               ;
0000ABr 1               ; calc1 = game_state.entities.y[index1];
0000ABr 1               ;
0000ABr 1  A4 rr            ldy     _index1
0000ADr 1  B9 rr rr         lda     _game_state+31,y
0000B0r 1  85 rr            sta     _calc1
0000B2r 1               ;
0000B2r 1               ; draw_ptr = screen_line_lookup[calc1];
0000B2r 1               ;
0000B2r 1  A2 00            ldx     #$00
0000B4r 1  A5 rr            lda     _calc1
0000B6r 1  0A               asl     a
0000B7r 1  90 02            bcc     L00FD
0000B9r 1  E8               inx
0000BAr 1  18               clc
0000BBr 1  69 rr        L00FD:    adc     #<(_screen_line_lookup)
0000BDr 1  85 rr            sta     ptr1
0000BFr 1  8A               txa
0000C0r 1  69 rr            adc     #>(_screen_line_lookup)
0000C2r 1  85 rr            sta     ptr1+1
0000C4r 1  A0 01            ldy     #$01
0000C6r 1  B1 rr            lda     (ptr1),y
0000C8r 1  85 rr            sta     _draw_ptr+1
0000CAr 1  88               dey
0000CBr 1  B1 rr            lda     (ptr1),y
0000CDr 1  85 rr            sta     _draw_ptr
0000CFr 1               ;
0000CFr 1               ; draw_ptr += game_state.entities.x[index1];
0000CFr 1               ;
0000CFr 1  A4 rr            ldy     _index1
0000D1r 1  B9 rr rr         lda     _game_state,y
0000D4r 1  18               clc
0000D5r 1  65 rr            adc     _draw_ptr
0000D7r 1  85 rr            sta     _draw_ptr
0000D9r 1  A9 00            lda     #$00
0000DBr 1  65 rr            adc     _draw_ptr+1
0000DDr 1  85 rr            sta     _draw_ptr+1
0000DFr 1               ;
0000DFr 1               ; calc1 = game_state.entities.type[index1];
0000DFr 1               ;
0000DFr 1  A2 00            ldx     #$00
0000E1r 1  A5 rr            lda     _index1
0000E3r 1  0A               asl     a
0000E4r 1  90 01            bcc     L00FC
0000E6r 1  E8               inx
0000E7r 1  85 rr        L00FC:    sta     ptr1
0000E9r 1  8A               txa
0000EAr 1  18               clc
0000EBr 1  69 rr            adc     #>(_game_state+93)
0000EDr 1  85 rr            sta     ptr1+1
0000EFr 1  A0 rr            ldy     #<(_game_state+93)
0000F1r 1  B1 rr            lda     (ptr1),y
0000F3r 1  85 rr            sta     _calc1
0000F5r 1               ;
0000F5r 1               ; *draw_ptr = get_entity_tile[calc1];
0000F5r 1               ;
0000F5r 1  A5 rr            lda     _draw_ptr+1
0000F7r 1  85 rr            sta     ptr1+1
0000F9r 1  A5 rr            lda     _draw_ptr
0000FBr 1  85 rr            sta     ptr1
0000FDr 1  A4 rr            ldy     _calc1
0000FFr 1  B9 rr rr         lda     _get_entity_tile,y
000102r 1  A0 00            ldy     #$00
000104r 1  91 rr            sta     (ptr1),y
000106r 1               ;
000106r 1               ; calc1 = game_state.entities.hp[index1];
000106r 1               ;
000106r 1  A4 rr            ldy     _index1
000108r 1  B9 rr rr         lda     _game_state+62,y
00010Br 1  85 rr            sta     _calc1
00010Dr 1               ;
00010Dr 1               ; *(++draw_ptr) = div_10_lookup [ calc1 ];
00010Dr 1               ;
00010Dr 1  E6 rr            inc     _draw_ptr
00010Fr 1  D0 02            bne     L009D
000111r 1  E6 rr            inc     _draw_ptr+1
000113r 1  A5 rr        L009D:    lda     _draw_ptr+1
000115r 1  85 rr            sta     ptr1+1
000117r 1  A5 rr            lda     _draw_ptr
000119r 1  85 rr            sta     ptr1
00011Br 1  A4 rr            ldy     _calc1
00011Dr 1  B9 rr rr         lda     _div_10_lookup,y
000120r 1  A0 00            ldy     #$00
000122r 1  91 rr            sta     (ptr1),y
000124r 1               ;
000124r 1               ; *(++draw_ptr) = mod_10_lookup [ calc1 ];
000124r 1               ;
000124r 1  E6 rr            inc     _draw_ptr
000126r 1  D0 02            bne     L00A3
000128r 1  E6 rr            inc     _draw_ptr+1
00012Ar 1  A5 rr        L00A3:    lda     _draw_ptr+1
00012Cr 1  85 rr            sta     ptr1+1
00012Er 1  A5 rr            lda     _draw_ptr
000130r 1  85 rr            sta     ptr1
000132r 1  A4 rr            ldy     _calc1
000134r 1  B9 rr rr         lda     _mod_10_lookup,y
000137r 1  A0 00            ldy     #$00
000139r 1  91 rr            sta     (ptr1),y
00013Br 1               ;
00013Br 1               ; };
00013Br 1               ;
00013Br 1  60               rts
00013Cr 1               
00013Cr 1               .endproc
```

Just like above, we can improve this function by addition intermediate variables:

```C
void draw_entity()
{
    calc1 = game_state.entities.y[index1];
    draw_ptr = screen_line_lookup[calc1];
    draw_ptr += game_state.entities.x[index1];
    calc1 = game_state.entities.type[index1];
    draw_ptr[0] = get_entity_tile[calc1];
    calc1 = game_state.entities.hp[index1];
    draw_ptr[1] = div_10_lookup [ calc1 ];
    draw_ptr[2] = mod_10_lookup [ calc1 ];
};
```

Lets take a look at the part of the current function for plotting entities and their HitPoint:

```assembly
0000F5r 1               ;
0000F5r 1               ; draw_ptr[0] = get_entity_tile[calc1];
0000F5r 1               ;
0000F5r 1  A5 rr            lda     _draw_ptr+1
0000F7r 1  85 rr            sta     ptr1+1
0000F9r 1  A5 rr            lda     _draw_ptr
0000FBr 1  85 rr            sta     ptr1
0000FDr 1  A4 rr            ldy     _calc1
0000FFr 1  B9 rr rr         lda     _get_entity_tile,y
000102r 1  A0 00            ldy     #$00
000104r 1  91 rr            sta     (ptr1),y
000106r 1               ;
000106r 1               ; calc1 = game_state.entities.hp[index1];
000106r 1               ;
000106r 1  A4 rr            ldy     _index1
000108r 1  B9 rr rr         lda     _game_state+62,y
00010Br 1  85 rr            sta     _calc1
00010Dr 1               ;
00010Dr 1               ; draw_ptr[1] = div_10_lookup [ calc1 ];
00010Dr 1               ;
00010Dr 1  A5 rr            lda     _draw_ptr+1
00010Fr 1  85 rr            sta     ptr1+1
000111r 1  A5 rr            lda     _draw_ptr
000113r 1  85 rr            sta     ptr1
000115r 1  A4 rr            ldy     _calc1
000117r 1  B9 rr rr         lda     _div_10_lookup,y
00011Ar 1  A0 01            ldy     #$01
00011Cr 1  91 rr            sta     (ptr1),y
00011Er 1               ;
00011Er 1               ; draw_ptr[2] = mod_10_lookup [ calc1 ];
00011Er 1               ;
00011Er 1  A5 rr            lda     _draw_ptr+1
000120r 1  85 rr            sta     ptr1+1
000122r 1  A5 rr            lda     _draw_ptr
000124r 1  85 rr            sta     ptr1
000126r 1  A4 rr            ldy     _calc1
000128r 1  B9 rr rr         lda     _mod_10_lookup,y
00012Br 1  A0 02            ldy     #$02
00012Dr 1  91 rr            sta     (ptr1),y
```

We see here that it is still not perfect. CC65 optimizer does not do here good tracking of registers and memory state even withing one basic block. This is the place that having more time I will look to improve in CC65 optimizer. Issues are:

1. Compiler internal **ptr1** is not getting modified, therefore copying it always from _draw_ptr should be removed by optimizer. CC65 does this optimization when "register" keyword is used (register unsigned char *draw_ptr), however ["register" cannot be defined as global](https://cc65.github.io/doc/coding.html#s13), which on one hand leads to code optimization and on other hand adds additional code for stack operations.
2. Our draw_ptr is already a pointer on zero page, therefore  "sta     (ptr1),y" could be replaced by "sta (_draw_ptr),y"

To help optimizer we need some additional higher-level optimizations.

## 12 - Inline functions, activate additional "register" keyword optimizations - 29 ticks (6% speedup)

CC65 does not inline leaf functions (there was a branch of CC65 long time ago doing this, but for some reason it was not merged to the "master branch") therefore to push performance further we can manually inline the time critical functions to the loop, to prevent function call and to help optimizer:

```c
void one_frame()
{
    register unsigned char *draw_ptr;         
    for (index1=0;index1 < NO_ENEMIES;++index1)
    {
        // inlined damage_enemy     
        calc1 = game_state.entities.hp[index1];
        if (calc1 > 0)
        {
            --calc1;
            game_state.entities.hp[index1] = calc1;
        }
    }
    
    for (index1=0;index1 < NO_ENTITIES;++index1)
    {
        // inlined drawing of entities
        calc1 = game_state.entities.y[index1];
        draw_ptr = screen_line_lookup[calc1];
        draw_ptr += game_state.entities.x[index1];
        calc1 = game_state.entities.type[index1];
        calc2 = get_entity_tile[calc1];
        calc1 = game_state.entities.hp[index1];
        calc3 = div_10_lookup [ calc1 ];
        calc4 = mod_10_lookup [ calc1 ];
        draw_ptr[0] = calc2;
        draw_ptr[1] = calc3;
        draw_ptr[2] = calc4;
    };    
}
```

What you see here that "register" keyword was used. Why to do it, when we already had *draw_ptr* in ZEROPAGE segment? Reason is that CC65 has some additional optimizations for "register" variables, that do not apply in the current CC65 version to user-defined zero-page variables. ["register" variables have additional overhead](https://cc65.github.io/doc/coding.html#s13) related to putting their value on stack when entering a function, but it's getting negligible if you define them out of loops. The code from the previous point looks much better now:

```assembly
0000FCr 1               ; calc1 = game_state.entities.type[index1];
0000FCr 1               ;
0000FCr 1  A4 rr            ldy     _index1
0000FEr 1  B9 rr rr         lda     _game_state+93,y
000101r 1  85 rr            sta     _calc1
000103r 1               ;
000103r 1               ; calc2 = get_entity_tile[calc1];
000103r 1               ;
000103r 1  A4 rr            ldy     _calc1
000105r 1  B9 rr rr         lda     _get_entity_tile,y
000108r 1  85 rr            sta     _calc2
00010Ar 1               ;
00010Ar 1               ; calc1 = game_state.entities.hp[index1];
00010Ar 1               ;
00010Ar 1  A4 rr            ldy     _index1
00010Cr 1  B9 rr rr         lda     _game_state+62,y
00010Fr 1  85 rr            sta     _calc1
000111r 1               ;
000111r 1               ; calc3 = div_10_lookup [ calc1 ];
000111r 1               ;
000111r 1  A4 rr            ldy     _calc1
000113r 1  B9 rr rr         lda     _div_10_lookup,y
000116r 1  85 rr            sta     _calc3
000118r 1               ;
000118r 1               ; calc4 = mod_10_lookup [ calc1 ];
000118r 1               ;
000118r 1  A4 rr            ldy     _calc1
00011Ar 1  B9 rr rr         lda     _mod_10_lookup,y
00011Dr 1  85 rr            sta     _calc4
00011Fr 1               ;
00011Fr 1               ; draw_ptr[0] = calc2;
00011Fr 1               ;
00011Fr 1  A5 rr            lda     _calc2
000121r 1  A0 00            ldy     #$00
000123r 1  91 rr            sta     (regbank+4),y
000125r 1               ;
000125r 1               ; draw_ptr[1] = calc3;
000125r 1               ;
000125r 1  A5 rr            lda     _calc3
000127r 1  C8               iny
000128r 1  91 rr            sta     (regbank+4),y
00012Ar 1               ;
00012Ar 1               ; draw_ptr[2] = calc4;
00012Ar 1               ;
00012Ar 1  A5 rr            lda     _calc4
00012Cr 1  C8               iny
00012Dr 1  91 rr            sta     (regbank+4),y
```

## Identify code critical places and rewrite them in assembly

There are still some optimizations to be done in C, but I will stop here. If you really need to optimize time-critical code then do it in assembly. If you do not know how to do it, ask the community for help and you will be surprised how helpful 8bit programmers are!

Critical is to find out which code requires improvement and ["code profilers"](https://en.wikipedia.org/wiki/Profiling_(computer_programming)) are to help you. Good 6502 emulators have more or less advanced code profilers built-in.

In case of using Altirra emulator:

* Enable debugger (F8)
* Load symbols ".loadsym game.lbl"
* Start Performance Analyzer to profile code
* Stop it at desired moment
* View how much specific instructions, basic blocks or functions take time
* Optimize identified bottlenecks in C
* Rewrite the time-critical code in assembly

## What else to optimize was not covered in this tutorial?

The example above does not cover all the cases. Here are a few that you will meet writing programs in CC65

### Dynamic memory allocations

Shortly, forget about malloc()/free() and use static buffers for everything. On 6502 we have so small memory and dynamic allocation so big overhead, that it makes both code size and speed very poor. Additionally deallocations of memory will lead to [memory fragmentation](https://en.wikipedia.org/wiki/Fragmentation_(computing)) that may prevent further allocations.

### Multidimensional arrays

2-dimensional arrays are often used to store game maps or game boards.

You could expect that multidimensional arrays are fast, because compiler knows the addresses for accessing data, however for some reason they are slow in CC65, even when the array fits in one 256 bytes page. Do not use multidimensional arrays (even two-dimensional). Use single dimensional array and address it with lookup table.

### Default runtime functions vs specialized functions

Some default CRT or library functions are huge. One commonly used is "printf" but it is so complex and big that you will really want to replace it with own function that is specialized and outputs characters or numbers on the screen.

Other one is for file operations. fopen, fwrite, fread, fclose have smaller and faster equivalents (open,write, read,close) from <fcntl.h> header file.

### Cache and precalc

The fastest code is one that is not executed so think if you really need to do calculations:

* Maybe you can not modify the state?
* Maybe you can cache the results?
* Maybe you can use lookup tables?
* Maybe you can calculate the result in advance?

### Increasing and decreasing variables

CC65 does not do high-level optimizations and to increase (or similarly decrease) value of variable use preincrement operator.
```C
    // instead of 
    a = a + 1;
   // or
    a += 1;
    // use:
    ++a;
```
This will be translated into a single "INC variable" assembler instruction.

### Drawing single pixels on the screen and interrupt handlers

Usually these are so time-critical, that should be done in assembler.

# Summary

We went through a lot of different cases and were able to improve the initially generated code from **528** to **29** ticks, which is **18 times**, almost to level of hand-written assembly!

When writing in C for 6502 remember that:

* Premature code optimization is root of all evil!
* Turn on optimization options in the compiler - you get it for free.
* Use proper data structures and algorithms from start  and add caching later.
* Organize data for fast access:
  * Use "struct of arrays" instead of "array of structs"
  * Keep arrays under 256 bytes whenever possible and use "unsigned char" to index them instead of pointers.
  * Use the smallest possible data types
* Use lookup tables whenever possible
* Later if needed
  * Get rid of C stack and passing parameters
  * Handle integer promotion and array access by calculating intermediate values before assignment
  * Use profiler and switch to assembler for time-critical code pieces

With just these steps your C code will be blazing-fast!