#ifndef F_CPU
#define F_CPU 1000000UL // or whatever may be your frequency
#endif

// uncomment at most on of the following lines for smoothing of the US readout
//#define SMOOTH_CMI      // channeling measurement interpreter by @Necktschnagge
//#define SMOOTH_AVR      // weighted average
#define SMOOTH_MOVING_AVR // moving average



#define TxDBit BIT(4)   // needs to be defined before including myserial.h

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "mydefs.h"
#include "myserial.h"
#ifdef SMOOTH_CMI
	#include "cmi.h"
#endif

//#define DEBUG_OUTPUT  // send current distance via UART


// wireing for this project. don't confuse port/bit and pin!
#define POSOUT  BIT(1)
#define NEGOUT  BIT(0)
#define TRIGGER BIT(2)
#define ECHO    BIT(3)
// already defined: TxDBit BIT(4)


constexpr uint8_t MAX_ECHO_HIGH{ 0x0B };
constexpr uint16_t DIST_OCT{ (MAX_ECHO_HIGH/2) << 8 };     // distance per octave

constexpr uint8_t OLD_AVR_PERCENTAGE{ 80 };
constexpr uint8_t NO_AVERAGE{ 20 };
#ifdef SMOOTH_MOVING_AVR
	volatile uint16_t avr_window[NO_AVERAGE];
#endif
volatile uint8_t curr_window_idx=0;
volatile uint8_t echo_timer_high;
volatile uint32_t distance=0;
volatile uint32_t avr_distance=0;

#ifdef SMOOTH_CMI
using TAnalyzer = analyzer::ChannellingMeasurementInterpreter<uint32_t, 4>;
TAnalyzer* channelPointer;
constexpr uint8_t fixed_comma_position{ 8 };
#endif

void init(){
	#ifdef SMOOTH_CMI
		// setup channels for denoising of US sensor
		static TAnalyzer::ConstDeltaConfiguration
			channelConfig((uint32_t(0xC0)) << fixed_comma_position /*channel width*/);
		channelConfig.weight_old = OLD_AVR_PERCENTAGE;
		/* assert: (weight_old + weight_new) * (max_measured_value << fixed_comma_position) < (1(U)LL << 32) */
		channelConfig.weight_new = 100-OLD_AVR_PERCENTAGE;
		channelConfig.initial_badness = 40;
		channelConfig.badness_reducer = 9;

		static TAnalyzer channels(channelConfig);
		channelPointer = &channels;
	#endif

	#ifdef SMOOTH_MOVING_AVR
		// initialize array for moving average calculation
		for (int i=0; i<NO_AVERAGE; i++){
			avr_window[i] = 0;
		}
	#endif

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
			distance = ((uint32_t) echo_timer_high) << 8 | TCNT0; //distance = run time of US sensor in timer ticks
			

#ifdef SMOOTH_CMI
			//denoise the distance with the channel object
			uint32_t localDistanceCopy = distance << fixed_comma_position;
			send_byte(distance >> 8); send_byte(distance & 0xFF);
			send_byte(channelPointer->input(localDistanceCopy));
			distance = channelPointer->output() >> fixed_comma_position;
#endif

#ifdef SMOOTH_AVR
			//denoise the distance with weighted average
			avr_distance = (avr_distance*OLD_AVR_PERCENTAGE/100 + distance*(100-OLD_AVR_PERCENTAGE));
			distance = avr_distance/100;
#endif

#ifdef SMOOTH_MOVING_AVR
			//denoise with true moving average
			avr_window[curr_window_idx++] = static_cast<uint16_t>(distance);
			curr_window_idx %= NO_AVERAGE;
			distance = 0;
			for (int i=0; i<NO_AVERAGE; i++){
				distance += avr_window[i];
			}
			distance /= NO_AVERAGE;
#endif

			// set tone
			uint8_t octave = (distance / DIST_OCT + 2) & 0x0F; // be shure to get a 4Bit value
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