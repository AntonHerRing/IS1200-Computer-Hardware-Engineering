#include <pic32mx.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ------------------------------------------------------------ */
/* Pin definitions for access to OLED control signals on chipKIT Uno32
*/

#define prtVddCtrl 	IOPORT_F
#define prtVbatCtrl IOPORT_F
#define prtDataCmd 	IOPORT_F
#define prtReset 	IOPORT_G
#define bitVddCtrl 	0x40
#define bitVbatCtrl 0x20
#define bitDataCmd 	0x10
#define bitReset 	0x200

/* ------------------------------------------------------------
	 Symbols describing the geometry of the display.*/

#define cbOledDispMax 	512 	//max number of bytes in display buffer
#define ccolOledMax 	128 	//number of display columns
#define crowOledMax 	32 		//number of display rows
#define cpagOledMax 	4 		//number of display memory pages

/* ------------------------------------------------------------ */

/* This array is the off-screen frame buffer used for rendering.
** It isn't possible to read back from the OLED display device,
** so display data is rendered into this off-screen buffer and then
** copied to the display.
*/
uint8_t rgbOledBmp[cbOledDispMax];

const uint8_t const font[128];

char textbuffer[4][16];

void OledPutBuffer(int cb, uint8_t * rgbTx);
uint8_t Spi2PutByte(uint8_t  bVal);

/* ------------------------------------------------------------ */
/*** OledHostInit
**
** Parameters:
** none
**
** Return Value:
** none
**
** Errors:
** none
**
** Description:
** Perform PIC32 device initialization to prepare for use
** of the OLED display.
** This example is hard coded for the chipKIT Uno32 and
** SPI2.
*/

void OledHostInit()
{
	/* Initialize SPI port 2.
	*/
	SPI2CON = 0;
	SPI2BRG = 15; 				//8Mhz, with 80Mhz PB clock
	SPI2STATCLR = PIC32_SPISTAT_SPIROV;		//ISR
	SPI2CONSET = PIC32_SPICON_CKP;
	SPI2CONSET = PIC32_SPICON_MSTEN;
	SPI2CONSET = PIC32_SPICON_ON;			//ISR

	/* Make pins RF4, RF5, and RF6 be outputs.*/
	PORTFSET = bitVddCtrl|bitVbatCtrl|bitDataCmd;

	PORTF = 0xFFFF;
	PORTG = (1 << 9);
	ODCF = 0x0;
	ODCG = 0x0;
	TRISFCLR = 0x70;
	TRISGCLR = 0x200;

	/* Make the RG9 pin be an output. On the Basic I/O Shield, this pin is tied to reset.*/
	PORTGSET = bitReset;
}

/* ------------------------------------------------------------ */
/*** OledDspInit
**
** Parameters:
** none
**
** Return Value:
** none
**
** Errors:
** none
**
** Description:
** Initialize the OLED display controller and turn the display on.
*/

void OledDspInit()
{
	/* We're going to be sending commands, so clear the Data/Cmd bit
	*/
	PORTFCLR = bitDataCmd;

	/* Start by turning VDD on and wait a while for the power to come up.
	*/
	PORTFCLR = bitVddCtrl;
	DelayMs(1);

	/* Display off command
	*/
	Spi2PutByte(0xAE);

	/* Bring Reset low and then high
	*/
	PORTGCLR = bitReset;
	DelayMs(1);

	PORTGSET = bitReset;
	DelayMs(1);

	/* Send the Set Charge Pump and Set Pre-Charge Period commands
	*/
	Spi2PutByte(0x8D);
	Spi2PutByte(0x14);

	Spi2PutByte(0xD9);
	Spi2PutByte(0xF1);

	/* Turn on VCC and wait 100ms
	*/
	PORTFSET = bitVbatCtrl;
	DelayMs(100);

	/* Send the commands to invert the display. This puts the display origin
	** in the upper left corner.
	*/
	Spi2PutByte(0xA1); 		//remap columns
	Spi2PutByte(0xC8); 		//remap the rows

	/* Send the commands to select sequential COM configuration. This makes the
	** display memory non-interleaved.
	*/
	Spi2PutByte(0xDA);		//set COM configuration command
	Spi2PutByte(0x20); 		//sequential COM, left/right remap enabled

	/* Send Display On command
	*/
	Spi2PutByte(0xAF);
}

/* ------------------------------------------------------------ */
/*** OledUpdate
**
** Parameters:
** none
**
** Return Value:
** none
**
** Errors:
** none
**
** Description:
** Update the OLED display with the contents of the memory buffer
*/

void OledUpdate(uint8_t * pb)
{
	int ipag;

	for (ipag = 0; ipag < cpagOledMax; ipag++)
	{
		PORTFCLR = bitDataCmd;

		/* Set the page address
		*/
		Spi2PutByte(0x22); //Set page command
		Spi2PutByte(ipag); //page number

		/* Start at the left column
		*/
		Spi2PutByte(0x00); //set low nybble of column
		Spi2PutByte(0x10); //set high nybble of column

		PORTFSET = bitDataCmd;

		/* Copy this memory page of display data.
		*/
		OledPutBuffer(ccolOledMax, pb);
		pb += ccolOledMax;
	}
}

/* ------------------------------------------------------*/
/** displays string
*/
void display_string(int line, char *s) 	//display text
{
	int i;
	if(line < 0 || line >= 4)
		return;
	if(!s)
		return;

	for(i = 0; i < 16; i++)
		if(*s) {
			textbuffer[line][i] = *s;
			s++;
		} else
			textbuffer[line][i] = ' ';
}

void display_update(void)
{
	int i, j, k;
	int c;
	for(i = 0; i < 4; i++) {
		PORTFCLR = 0x10;
		Spi2PutByte(0x22);
		Spi2PutByte(i);

		Spi2PutByte(0x0);
		Spi2PutByte(0x10);

		PORTFSET = 0x10;

		for(j = 0; j < 16; j++) {
			c = textbuffer[i][j];
			if(c & 0x80)
				continue;

			for(k = 0; k < 8; k++)
				Spi2PutByte(font[c*8 + k]);
		}
	}
}

/* ------------------------------------------------------------ */
/*** OledPutBuffer
**
** Parameters:
** cb - number of bytes to send/receive
** rgbTx - pointer to the buffer to send
**
** Return Value:
** none
**
** Errors:
** none
**
** Description:
** Send the bytes specified in rgbTx to the slave.
*/
void OledPutBuffer(int cb, uint8_t * rgbTx)
{
	int ib;
	uint8_t bTmp;

	/* Write/Read the data
	*/
	for (ib = 0; ib < cb; ib++)
	{
		/* Wait for transmitter to be ready
		*/
		while((SPI2STAT & PIC32_SPISTAT_SPITBE) == 0);

		/* Write the next transmit byte.
		*/
		SPI2BUF = *rgbTx++;

		/* Wait for receive byte.
		*/
		while((SPI2STAT & PIC32_SPISTAT_SPIRBF) == 0);
		bTmp = SPI2BUF;
	}
}

/* ------------------------------------------------------------ */
/*** Spi2PutByte
**
** Parameters:
** bVal - byte value to write
**
** Return Value:
** Returns byte read
**
** Errors:
** none
**
** Description:
** Write/Read a byte on SPI port 2
*/
uint8_t Spi2PutByte(uint8_t  bVal)
{
	uint8_t bRx;

	/* Wait for transmitter to be ready
	*/
	while ((SPI2STAT & PIC32_SPISTAT_SPITBE) == 0);

	/* Write the next transmit byte.
	*/
	SPI2BUF = bVal;

	/* Wait for receive byte.
	*/
	while ((SPI2STAT & PIC32_SPISTAT_SPIRBF) == 0);

	/* Put the received byte in the buffer.
	*/
	bRx = SPI2BUF;

	return bRx;
}