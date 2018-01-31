#ifndef F_CPU
#define F_CPU 1000000UL // or whatever may be your frequency
#endif

#define TxDBit BIT(4)   // needs to be defined before including myserial.h

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "mydefs.h"
#include "myserial.h"

//#define DEBUG_OUTPUT  // send current distance via UART
//#define DEBUG_TONE    // use artificial distance -> produce smoothly falling tone. (only if something is near ultrasonic sensor)


// wireing for this project. don't confuse port/bit and pin!
#define POSOUT  BIT(1)
#define NEGOUT  BIT(0)
#define TRIGGER BIT(2)
#define ECHO    BIT(3)
// already defined: TxDBit BIT(4)


#define MAX_ECHO_HIGH      0x0B
#define DIST_OCT           ((MAX_ECHO_HIGH/2)<<8)     // distance per octave


volatile uint8_t echo_timer_high;
volatile uint16_t distance=0;


void init(){
	// configure TONE generator with Timer 1
		SETOUTPUT(POSOUT);
		SETOUTPUT(NEGOUT);

		#if false   // PLL is not really necessary. internal system clock is fast enough
		// activate PLL clock source
			PLLCSR |= (1<<LSM);                 // low speed mode (=32MHz). for compatibility with low operating volatges (<2.7V)
			PLLCSR |= (1<<PLLE);                // enable PLL
			_delay_us(100);                     // wait for PLL to become stable
			while(!(PLLCSR & (1<<PLOCK))) {}    // wait for PLL to be locked
			PLLCSR |= (1<<PCKE);                // use PLL as peripheral clock (async mode)
			// clock source for Timer 1 is now running with 32MHz
		#endif

		// setup timer 1
			#define TIMER1_SETTINGS (1<<CTC1 | 1<<PWM1A | 0<<COM1A1 | 1<<COM1A0)
			//                      clear timer/counter on Compare Match with OCR1C
			//                                activate PWM mode (compare against OCR1A)
			//                                           activate output according to compare
			
			// initial timer values:
				//uint8_t timer1prescaleExp = 0<<CS13 | 0<<CS12 | 0<<CS11 | 0<<CS10;
				uint8_t timer1prescaleExp = 0xF<<CS10; // must be a 4 Bit value
				// prescale factor = 2^(prescaleExp - 1)

				OCR1C = 128;
				OCR1A = 64;

				// this should result in a sound/flicker of
				//   32MHz / 2^(0xF - 1) / 128
				// = 32MHz / 2^(14)      / 128
				// = 32MHz / 16384       / 128
				// = 15.2 Hz
				// in case of low speed mode (32MHz) PLL (async mode) as clock source or
				//   1MHz / 2^(0xF - 1) / 128
				// = 1MHz / 2^(14)      / 128
				// = 1MHz / 16384       / 128
				// = 0.48 Hz
				// in case of 1MHz system clock (sync mode) as clock source
			
			TCCR1 = TIMER1_SETTINGS | timer1prescaleExp;

	// configure ultrasonic distance measurement
		SETOUTPUT(TRIGGER);
		SETINPUT(ECHO);
		CLEARBIT(ECHO);   // disable pullup resistor for this port

		// enable interrupts for ECHO signal
			GIMSK |= 1<<PCIE;    // globaly enable pin change interrupt
			PCMSK |= (ECHO);     // enable pin change interrupt for ECHO pin

		// setup timer 0
			TIMSK |= (1<<TOIE0);    // enable interrupt on overflow of timer 0
			//#define ACTIVATE_ECHO_TIMER (0<<CS02 | 1<<CS01 | 0<<CS00) // activate Timer0, internal source, /8 prescaling -> 125kHz
			#define ACTIVATE_ECHO_TIMER (0<<CS02 | 0<<CS01 | 1<<CS00)   // activate Timer0, internal source, /1 prescaling -> 1MHz
			#define STOP_ECHO_TIMER (~(1<<CS02 | 1<<CS01 | 1<<CS00))
			TCCR0B |= ACTIVATE_ECHO_TIMER;


	// globally enable interrupts
	sei();

	// configure serial communication (send only)
	init_send();
}


// handle ECHO timer overflow. implements 16Bit timer
ISR(TIMER0_OVF_vect){
	echo_timer_high++;
	if (echo_timer_high >= MAX_ECHO_HIGH){ //no sensible measurement
		TCCR0B &= STOP_ECHO_TIMER;          // stop timer
	}
}


// handle ECHO signal
ISR(PCINT0_vect){
	if(READBIT(ECHO)){   // signal send. reset and start timer
		TCCR0B &= STOP_ECHO_TIMER;    // stop timer
		echo_timer_high = 0;          // reset both timer bytes
		TCNT0 = 0;
		GTCCR |= (1<<PSR0);           // clear internal prescaler counter
		TCCR0B |= ACTIVATE_ECHO_TIMER;   // start timer

	} else {             // signal received. stop and read timer
		TCCR0B &= STOP_ECHO_TIMER;    // stop timer
		
		if (TIFR & (1<<TOV0)) {       // an overflow occured recently, that has not yet been processed
			echo_timer_high++;         // step high byte of timer
			TIFR = 1<<TOV0;            // clear timer overflow interrupt flag
												// this is a strange way to clear a bit. see datasheet.
		}

		if (echo_timer_high < MAX_ECHO_HIGH){  // otherwise assume timeout
			#ifdef DEBUG_TONE
				distance++; // for debugging only
			#else
				distance = ((uint16_t) echo_timer_high) << 8 | TCNT0;
			#endif
			// set tone
			uint8_t octave = (distance / DIST_OCT + 2) & 0xF; // be shure to get a 4Bit value
			uint8_t Tperiod = (uint32_t)(distance % DIST_OCT) * 128 / DIST_OCT + 128;

			TCCR1 = TIMER1_SETTINGS | octave;
			OCR1C = Tperiod;
			OCR1A = Tperiod/2;

			#ifdef DEBUG_OUTPUT
				// send debugging output
				send_byte(distance>>8);
				send_byte(distance);
			#endif
		}
	

	}
}



void trigger_us(){
	CLEARBIT(TRIGGER);
	_delay_us(20);          // stay low for at least 20 μs
	SETBIT(TRIGGER);
}




int main(void){
	init();

	while(1){
		trigger_us();
		_delay_ms(1);
		// ECHO pulse should have started by now
		while(READBIT(ECHO)) {} // wait until ECHO pulse is over
		_delay_ms(5);           // to be on the save side
	}
	return 0;
}