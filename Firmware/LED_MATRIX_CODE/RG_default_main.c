/*



*/


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

#define CLK				0
#define CLR				3
#define LATCH			2
#define DATA			1
#define EN				4


volatile uint16_t count;
volatile uint8_t temp;
volatile uint8_t spiTemp;
volatile uint16_t temp16;
volatile uint8_t countb;
volatile uint16_t line; 
volatile uint8_t row;
volatile uint8_t frameBufferIndex;

volatile uint8_t image[] = {
  1, 2, 3, 2,  0, 0, 0, 3,
  0, 1, 0, 0,  0, 0, 3, 0,
  0, 0, 1, 0,  0, 3, 0, 0,
  0, 0, 0, 1,  3, 0, 0, 0,

  0, 0, 0, 3,  1, 0, 0, 0,
  0, 0, 3, 0,  0, 1, 0, 0,
  0, 3, 0, 0,  0, 0, 1, 0,
  3, 0, 0, 0,  0, 0, 0, 1,
};


volatile uint8_t rowMap[] = { 7, 6, 5, 4,  0, 1, 2, 3 };

ISR(SIG_SPI) 
{
  if(!(PINB&0x04))
  {
    spiTemp = SPDR;      					// read spi data register
/*    SPDR = image[((((frameBufferIndex+1)%64) / 8) * 8) + ((63 - ((frameBufferIndex+1)%64)) % 8)];*/
    SPDR = image[((((frameBufferIndex+1)&63))&(~7))+((63-(frameBufferIndex+1)&63)&7)];
    //image[frameBufferIndex] = spiTemp;		// load value to frame buffer
/*    image[((frameBufferIndex / 8) * 8) + ((63 - frameBufferIndex) % 8)] = spiTemp;  // flip display*/
    image[(((frameBufferIndex))&(~7))+((63-(frameBufferIndex)&63)&7)] = spiTemp;
    frameBufferIndex++;					// increment frame buffer index (pixel number)
/*    frameBufferIndex %= 64;					// wrap at 64*/
    frameBufferIndex &= 63;
  }
}


void delay_ms(uint16_t x);					// general purpose delay

void ioinit (void);						// initializes IO

// this is the function that shifts 16 bits out to the 74hc595 shift registers
// it is inlined to speed things up
void inline shiftLine(uint16_t line, uint8_t rowNum);


int main (void)
{
  uint8_t i;
  uint8_t startupPattern;
  ioinit ();

  SPCR = (1 << SPE) | (1 << SPIE);
  sei();

  cbi(PORTC, CLR);
  delay_ms(1);
  sbi(PORTC, CLR);
  cbi(PORTC, EN);

  row = 0;
  PORTD = 0;

  for (i = 0; i < 64; i++)
    image[i] = 0;

  frameBufferIndex = 6;
  startupPattern = 1;


  line = 0x00ff;
  for (row = 0; row < 8; row++){
    shiftLine(line, row);
    delay_ms(700);
  }

  line = 0xff00;
  for (row = 0; row < 8; row++){
    shiftLine(line, row);
    delay_ms(700);
  }


  line = 0xffff;
  for (row = 0; row < 8; row++){
    shiftLine(line, row);
    delay_ms(700);
  }


  for (;;){	//LED_off;

    // check cs status, reset frame buffer index to 0 if set

    if (PINB & 0x04)
      frameBufferIndex = 0;

    for (row = 0; row < 8; row++){
      line = 0;
      for (i = 0; i < 8; i++){

        switch (image[i + (8 * row)]){
          case 1: line |= (0x0001 << i); break;
          case 2: line |= (0x0100 << i); break;
          case 3: line |= (0x0101 << i); break; 
        }

      }
      shiftLine(line, row);
    }

  }

  return (0);
}

void ioinit (void) /* Note [5] */
{
  DDRB |= (1 << 4);
  DDRD = 0xFF;  // atmega8 LEDs
  PORTD = 0x01;
  DDRC = 0x1F;
  sbi(PORTC, CLK);
  sbi(PORTC, CLR);
  sbi(PORTC, DATA);
  sbi(PORTC, LATCH);
  sbi(PORTC, EN);
}

void inline shiftLine(uint16_t line, uint8_t rowNum){
  uint8_t i;
  //sbi(PORTC, EN);
  cbi(PORTC, LATCH);
  for(i = 0; i < 16; i++){

    cbi(PORTC, CLK);
    //cbi(PORTC, LATCH);

    if (line & (1 << i))
      sbi(PORTC, DATA);
    else
      cbi(PORTC, DATA);

    sbi(PORTC, CLK);
    //sbi(PORTC, LATCH);
  }
  sbi(PORTC, EN);
  sbi(PORTC, LATCH);
  PORTD = (1 << rowMap[rowNum]);
  cbi(PORTC, EN);
}

//General short delays
void delay_ms(uint16_t x)
{
  uint8_t y, z;
  for ( ; x > 0 ; x--){
    for ( y = 0 ; y < 4 ; y++){
      for ( z = 0 ; z < 40 ; z++){
        asm volatile ("nop");
      }


    }
  }
}

