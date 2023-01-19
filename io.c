#include <pic32mx.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void dispinnit(void)
{
	OledHostInit();
	OledDspInit();
	return;
}

void timerinnit(void)
{
	T2CON = 0x0;
	TMR2 = 0x0;				// init to 0
	PR2 = 0x04e2;			// sets to (3125)
	T2CONSET = 0x8060;		// sets prescale to 1:64

	IEC(0) |= 0x10000;
	IEC(0) |= 0x800;
	IPC(4) |= 0x1C;
	IPC(4) |= 0x3;

	enable_interrupt();
	return;
}
void ioinnit(void)
{
	TRISE = 0x00;
	PORTE = 0x0;

	TRISDSET = 0xfe0; 	//btn 4 - 3 and sw2
	TRISFSET = 0x2;		//btn 1
	PORTDCLR = 0xffff;

	IPCSET(2) = 0x1F; 	//timer
	IPCSET(2) = 0x1b00;	//sw2?
}
int getbtns(void)
{
	int btns = ((PORTF >> 1) & 0x1);
	btns |= ((PORTD >> 4) & 0xe);	//puts upper 3 btns bits to lsb		//masks out unwanted bits not in 0 - 2 & 0111 = 7
	//btns |= ((PORTF >> 1) & 0x1);
	return btns;
}

void DelayMs(int x)
{
	int counter = 0;
	TMR2 = 0x0;

	while(counter < x)
	{
		if(IFS(0) & 0x100)
		{
			IFSCLR(0) = 0x100;
			counter++;
		}
	}
	return;
}