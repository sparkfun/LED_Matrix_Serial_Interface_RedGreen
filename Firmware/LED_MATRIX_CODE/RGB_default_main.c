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

volatile uint8_t spiTemp;
volatile uint8_t lineByte[3]; 
volatile uint32_t red[8]; 
volatile uint32_t green[8];
volatile uint32_t blue[8];
volatile uint8_t row;
volatile uint8_t frameBufferIndex;


volatile uint32_t red[] 	= {0x000001, 0x040000, 0x000008, 0x200000, 0x000040, 0x010000, 0x000200, 0x002000}; 
volatile uint32_t green[]	= {0x800000, 0x000004, 0x100000, 0x000020, 0x000080, 0x008000, 0x000400, 0x004000};
volatile uint32_t blue[]	= {0x000002, 0x080000, 0x000010, 0x400000, 0x020000, 0x000100, 0x001000, 0x000800};

volatile uint8_t image[] = {		
  1,  1,  1,  1,  1,  1,  1,  1,
  0,  1,  0,  0,  0,  0,  3,  0,
  0,  0,  1,  0,  0,  3,  0,  0,
  0,  0,  0,  1,  3,  0,  0,  0,
  0,  0,  0,  3,  1,  0,  0,  0,
  0,  0,  3,  0,  0,  1,  0,  0,
  0,  3,  0,  0,  0,  0,  1,  0,
  3,  0,  0,  0,  0,  0,  0,  1,
};

ISR (SIG_SPI) 
{
  if (!(PINB & 0x04))
  {
    spiTemp = SPDR;      			// read spi data register
/*    SPDR = image[(frameBufferIndex+1)%64];      |+ Write SPI Data register +|*/
    SPDR = image[(frameBufferIndex+1)&63];
    image[frameBufferIndex] = spiTemp;		// load value to frame buffer
    frameBufferIndex++;				// increment frame buffer index (pixel number)
/*    frameBufferIndex %= 64;			// wrap at 64*/
    frameBufferIndex &= 63;
  }
}


void delay_ms(uint16_t x);					// general purpose delay

void ioinit (void);						// initializes IO

// this is the function that shifts 16 bits out to the 74hc595 shift registers
// it is inlined to speed things up
void inline shiftLine(volatile uint8_t[], volatile uint8_t rowNum);

int main (void)
{
  uint8_t i;
  uint32_t line;

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
  i = 0;

  lineByte[0] = 0xff;
  lineByte[1] = 0xff;
  lineByte[2] = 0xff;

  for (row = 0; row < 8; row++){
    shiftLine(lineByte, row);
    delay_ms(700);
  }

  for (row = 0; row < 8; row++){
    shiftLine(lineByte, row);
    delay_ms(700);
  }


  for (row = 0; row < 8; row++){
    shiftLine(lineByte, row);
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
          case 0: line |= 0; break;
          case 1: line |= red[i]; break;
          case 2: line |= green[i]; break;
          case 3: line |= blue[i]; break; 
          case 4: line |= (red[i] | green[i]); break;
          case 5: line |= (green[i] | blue[i]); break;
          case 6: line |= (blue[i] | red[i]); break; 
          case 7: line |= (red[i] | green[i] | blue[i]); break;
        }
        //line |= red[i];
      }
      lineByte[0] = line;
      lineByte[1] = line >> 8;
      lineByte[2] = line >> 16;
      shiftLine(lineByte, row);
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

void inline shiftLine(volatile uint8_t byte[], volatile uint8_t rowNum){
  uint8_t i, j;

  cbi(PORTC, LATCH);
  //sbi(PORTC, EN);
  for (j = 0; j < 3; j++){
    for(i = 0; i < 8; i++){

      cbi(PORTC, CLK);
      if (byte[j] & (1 << i))
        sbi(PORTC, DATA);
      else
        cbi(PORTC, DATA);

      sbi(PORTC, CLK);

    }
  }

  sbi(PORTC, EN);
  sbi(PORTC, LATCH);
  PORTD = (1 << rowNum);//(1 << rowMap[rowNum]);
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

