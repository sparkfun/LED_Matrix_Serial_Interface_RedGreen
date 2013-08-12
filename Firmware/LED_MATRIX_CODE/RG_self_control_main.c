/*



*/


#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/signal.h>
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

volatile	uint8_t i;
volatile	uint8_t t;
volatile 	uint8_t index;


volatile uint8_t image[] = {		1, 2, 3, 2,  0, 0, 0, 3,
									0, 1, 0, 0,  0, 0, 3, 0,
									0, 0, 1, 0,  0, 3, 0, 0,
									0, 0, 0, 1,  3, 0, 0, 0,
									
									0, 0, 0, 3,  1, 0, 0, 0,
									0, 0, 3, 0,  0, 1, 0, 0,
									0, 3, 0, 0,  0, 0, 1, 0,
									3, 0, 0, 0,  0, 0, 0, 1,
				   };
				   
				   
volatile uint8_t rowMap[] = { 7, 6, 5, 4,  0, 1, 2, 3 };

void delay_ms(uint16_t x);					// general purpose delay

void ioinit (void);						// initializes IO

// this is the function that shifts 16 bits out to the 74hc595 shift registers
// it is inlined to speed things up
void inline shiftLine(uint16_t line, uint8_t rowNum);

INTERRUPT (SIG_SPI) 
{
	/*spiTemp = SPDR;      					// read spi data register
	image[frameBufferIndex] = spiTemp;		// load value to frame buffer
		frameBufferIndex++;					// increment frame buffer index (pixel number)
	frameBufferIndex %= 64;					// wrap at 64*/
	
}


// timer 2 is audio interrupt timer
INTERRUPT (SIG_OVERFLOW2)
{
	for (row = 0; row < 8; row++){
		line = 0;
		for (i = 0; i < 8; i++){
			
			switch (image[i + (8 * row)]){
				case 1: line |= (0x0001 << i); break;
				case 2: line |= (0x0100 << i); break;
				case 3: line |= (0x0101 << i); break; 
			}
		
			/*if (image[i + (8 * row)] == 1)
				line |= (1 << i);
			else if (image[i + (8 * row)] == 2)
				line |= (1 << (i +8));
			else if (image[i + (8 * row)] == 3)
				line |= ( (1 << i) | (1 << (i +8)));*/
			
		}
		
		
		shiftLine(line, row);
	}
}



int main (void)
{
	uint8_t count;

    ioinit ();

	//SPCR = (1 << SPE) | (1 << SPIE);
	
	TCNT2 = 0;  // frame timer
	TCCR2 = 0x04;
	
	TIMSK = (1<<TOIE0)|(1<<TOIE2);	// interrupt enable timer0 and 2
	
	
	
	cbi(PORTC, CLR);
	delay_ms(1);
	sbi(PORTC, CLR);
	cbi(PORTC, EN);
	
	row = 0;
	PORTD = 0;
	
	for (i = 0; i < 64; i++)
		image[i] = 2;
		
	sei();
	
	for (;;){	//LED_off;
		

		index += count;
		count++ ;
		index %= 64;
		image[index] += 1;
		image[index] %= 4;        // do some interesting pattern
		delay_ms(200);          // not really 50 ms becuase of the interrupt
			


	}

    return (0);
}

void ioinit (void) /* Note [5] */
{
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

