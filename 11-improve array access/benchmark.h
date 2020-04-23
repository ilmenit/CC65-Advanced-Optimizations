#include <stdio.h>

void start_benchmark(void)
{
	asm("SEI");
	// wait for vblank
	asm("lda $14");
wvb:
	asm("cmp $14");
	asm("beq %g", wvb);
	
	// reset timer
	asm("lda #0");
	asm("sta $13");
	asm("sta $14");	
	asm("CLI");
}

unsigned int ticks;

void end_benchmark(void)
{
	asm("SEI");
	asm("lda $14");
	asm("sta %v", ticks);
	asm("lda $13");
	asm("sta %v+1", ticks);
	asm("CLI");
	printf("%u ticks", ticks);	
}
