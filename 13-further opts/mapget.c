#include "benchcommon.h"
#include <conio.h>
#include <stdlib.h>

#define MAP_SIZE_X 20
#define MAP_SIZE_Y 12

#define MapSet1(x,y,a) map_lookup_ptr[y][x]=(a);
#define MapGet1(x,y) map_lookup_ptr[y][x]

#define MapSet2(x,y,a) map[(x)+map_lookup[(y)]]=(a);
#define MapGet2(x,y) map[(x)+map_lookup[(y)]]

#define MapSet3(x,y,a) array_index = (x)+map_lookup[(y)];\
                       array_value = (a);\
                       map[array_index]=array_value;
#define MapGet3(x,y,r) array_index = (x)+map_lookup[(y)];\
                       r=map[array_index];

typedef unsigned char byte;
byte array_index;
byte array_value;
byte map[MAP_SIZE_X * MAP_SIZE_Y];

const byte map_lookup[MAP_SIZE_Y] =
{
	MAP_SIZE_X * 0,
	MAP_SIZE_X * 1,
	MAP_SIZE_X * 2,
	MAP_SIZE_X * 3,
	MAP_SIZE_X * 4,
	MAP_SIZE_X * 5,
	MAP_SIZE_X * 6,
	MAP_SIZE_X * 7,
	MAP_SIZE_X * 8,
	MAP_SIZE_X * 9,
	MAP_SIZE_X * 10,
	MAP_SIZE_X * 11
};

byte *map_lookup_ptr[MAP_SIZE_Y] =
{
	&map[MAP_SIZE_X * 0],
	&map[MAP_SIZE_X * 1],
	&map[MAP_SIZE_X * 2],
	&map[MAP_SIZE_X * 3],
	&map[MAP_SIZE_X * 4],
	&map[MAP_SIZE_X * 5],
	&map[MAP_SIZE_X * 6],
	&map[MAP_SIZE_X * 7],
	&map[MAP_SIZE_X * 8],
	&map[MAP_SIZE_X * 9],
	&map[MAP_SIZE_X * 10],
	&map[MAP_SIZE_X * 11]
};

byte x,y,i,temp;

void test1()
{
	for (i=0;i<100;++i)
	{
		for (y=0;y<MAP_SIZE_Y;++y)
		{
			for (x=0;x<MAP_SIZE_X;++x)
			{
				temp  = MapGet1(x,y);
				MapSet1(x,y,temp + 1);
			}
		}
	}
}

void test2()
{
	for (i=0;i<100;++i)
	{
		for (y=0;y<MAP_SIZE_Y;++y)
		{
			for (x=0;x<MAP_SIZE_X;++x)
			{
				temp  = MapGet2(x,y);
				MapSet2(x,y,temp + 1);
			}
		}
	}
}

void test3()
{
	for (i=0;i<100;++i)
	{
		for (y=0;y<MAP_SIZE_Y;++y)
		{
			for (x=0;x<MAP_SIZE_X;++x)
			{
				MapGet3(x,y,temp);
				MapSet3(x,y,temp + 1);
			}
		}
	}
}

void main(void)
{
		puts("1");
	    start();
		test1();
		end();
		
		puts("2");
	    start();
		test2();
		end();
		
		puts("3");
	    start();
		test3();
		end();
	
		asm(" jmp *");
}