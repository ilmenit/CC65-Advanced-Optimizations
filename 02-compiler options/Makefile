CL65_OPTIMIZATION_FLAGS=-Osir -Cl

all:	game.xex

run:	game.xex
	atari800 -run game.xex

clean:
	-rm game.xex
	-rm game.lst
	-rm game.lbl
	-rm game.o

game.xex game.lst game.lbl game.o: game.c benchmark.h
	cl65 -t atari  -Ln game.lbl ${CL65_OPTIMIZATION_FLAGS} --listing game.lst --add-source -o game.xex game.c
