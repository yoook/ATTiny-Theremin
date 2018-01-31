
/* sending and recieving is implemented independently. However, both share the same
* PARITY and BAUDRATE settings (this can be refactured easily, if required)
* 
* SEND:
* define TxDBit, TxDPort and TxDDDRRegister, PARITY and BAUDRATE as required before
* including this library (reasonable defaults exist: Bit4, PORTB, DDRB, no parity, 9600bps)
* call init_send() once and send_byte(unsigned char) as required.
*
* RECEIVING:
* define RxDBit, RxDPort, RxDPullup and RxDDDRRegister, PARITY and BAUDRATE as required before
* including this library (reasonable defaults exist: Bit4, PINB, PORTB, DDRB, no parity, 9600bps)
* call init_recv() once and int recv_byte() as required.
* recv_byte returns -1 if parity check failed or no START bit is detected and
* otherwise the received byte.
* You are responsible yourself for calling recv_byte() at the right moment, that is at
* the beginning of the START bit.
*
* TWEAKING:
* BITDELAY should be the duration of one bit minus 1 CPU cycle. 
* FRSTBITDELAY should be the delay between the start of the START bit and the
* middle of the first DATA bit minus a few CPU cycles. It should be a little bit
* less then 1.5 times the duration of one bit.
* Both are calculated automatically but can be overwritten by defining them.
* NOCHECKSTARTBIT can be defined to omit the early return of recv_byte() if no
* start bit was detected.
*
* CONSTRAINTS:
* while send_byte() is running, the TxDPort ist reset to its initial state several times.
* This might be an issue, if send_byte() is interrupted and some bits of TxDPort are
* changed in the interrupt handler.
* If send_byte() or recv_byte() are interrupted, their timing will be shifted and might
* result in the wrong data being send/received.
* Only one sending and only one receiving connection are supported at the moment.
*/

#ifndef __myserial_h__
#define __myserial_h__


#ifndef F_CPU
#error "F_CPU needs to be defined before incude!"
#endif

#include <util/delay.h>


// general
#ifndef PARITY
#define PARITY 0        // 0->none, 1->odd, 2->even
#endif


#ifndef BAUDRATE
#define BAUDRATE 9600
#endif


// transmit
#ifndef TxDBit
#define TxDBit (1<<4)   // bit 4
#endif

#ifndef TxDPort
#define TxDPort         PORTB
#define TxDDDRRegister  DDRB
#endif
#ifndef TxDDDRRegister
#error "You mustn't define TxDPort without defining TxDDDRRegister!"
#endif



// receive
#ifndef RxDBit
#define RxDBit (1<<3)   // bit 3
#endif

#ifndef RxDPort
#define RxDPort         PINB
#define RxDPullup       PORTB
#define RxDDDRRegister  DDRB
#endif
#if ! (defined(RxDDDRRegister) && defined(RxDPullup))
#error "You mustn't define RxDPort without defining RxDDDRRegister and RxDPullup!"
#endif





// timing
#ifndef BITDELAY
// (duration of one bit) - (cycles needed to prepare next bit)*(cycle time)
// (factors are rearanged to minimize rounding errors)
#define BITDELAY ((1000000UL / BAUDRATE) - (1*1000000UL)/F_CPU)    
//#define BITDELAY 103  // for 1MHz and 1 cycle delay
#endif

#ifndef FRSTBITDELAY
// delay between call of recv_byte (that should be at beginning of the START bit)
// and the first measurement of the first DATA bit (that should be in the middle
// of this bit)
// it is 1.5 times the duration of one bit minus 10 CPU cycles that are spend with
// some overhead
#define FRSTBITDELAY ((1500000UL / BAUDRATE) - (10*1000000UL)/F_CPU)
#endif



void init_send(){
	TxDDDRRegister |= TxDBit;  // set pin as output
	TxDPort        |= TxDBit;  // default state is HIGH
}

void init_recv(){
	RxDDDRRegister &= ~RxDBit; // set pin an input
	RxDPullup      &= ~RxDBit; // disable Pullup resistor
}


void send_byte(unsigned char b){
	unsigned char b0, b1, b2, b3, b4, b5, b6, b7, zero, one;
	unsigned char prevPort = TxDPort;
	zero = prevPort & ~(TxDBit);
	one  = prevPort |  (TxDBit);
	if (b&1)     b0 = one;  else b0 = zero;
	if (b&1<<1)  b1 = one;  else b1 = zero;
	if (b&1<<2)  b2 = one;  else b2 = zero;
	if (b&1<<3)  b3 = one;  else b3 = zero;
	if (b&1<<4)  b4 = one;  else b4 = zero;
	if (b&1<<5)  b5 = one;  else b5 = zero;
	if (b&1<<6)  b6 = one;  else b6 = zero;
	if (b&1<<7)  b7 = one;  else b7 = zero;

	#if PARITY == 0
		TxDPort = zero;            // start bit
		_delay_us(BITDELAY);
		TxDPort = b0;
		_delay_us(BITDELAY);
		TxDPort = b1;
		_delay_us(BITDELAY);
		TxDPort = b2;
		_delay_us(BITDELAY);
		TxDPort = b3;
		_delay_us(BITDELAY);
		TxDPort = b4;
		_delay_us(BITDELAY);
		TxDPort = b5;
		_delay_us(BITDELAY);
		TxDPort = b6;
		_delay_us(BITDELAY);
		TxDPort = b7;
		_delay_us(BITDELAY);
		TxDPort = one;          // 1.5 stop bits
		_delay_us(1.5 * BITDELAY);

	#else
		unsigned char parity = b0 ^ b1 ^ b2 ^ b3 ^ b4 ^ b5 ^ b6 ^ b7;
		if(PARITY==1){
			parity^=TxDBit;
		}
		parity |= prevPort;


		TxDPort = zero;            // start bit
		_delay_us(BITDELAY);
		TxDPort = b0;
		_delay_us(BITDELAY);
		TxDPort = b1;
		_delay_us(BITDELAY);
		TxDPort = b2;
		_delay_us(BITDELAY);
		TxDPort = b3;
		_delay_us(BITDELAY);
		TxDPort = b4;
		_delay_us(BITDELAY);
		TxDPort = b5;
		_delay_us(BITDELAY);
		TxDPort = b6;
		_delay_us(BITDELAY);
		TxDPort = b7;
		_delay_us(BITDELAY);
		TxDPort = parity;
		_delay_us(BITDELAY);
		TxDPort = one;          // 1.5 stop bits
		_delay_us(1.5 * BITDELAY);
	#endif
}

int recv_byte(){
	#ifndef NOCHECKSTARTBIT
	if(RxDPort & RxDBit){
		return -1;     // this was not the START bit
	}
	#endif

	unsigned char b0, b1, b2, b3, b4, b5, b6, b7, parity, oddeven, retval;
	#if PARITY == 0
		_delay_us(FRSTBITDELAY);
		b0 = RxDPort;
		_delay_us(BITDELAY);
		b1 = RxDPort;
		_delay_us(BITDELAY);
		b2 = RxDPort;
		_delay_us(BITDELAY);
		b3 = RxDPort;
		_delay_us(BITDELAY);
		b4 = RxDPort;
		_delay_us(BITDELAY);
		b5 = RxDPort;
		_delay_us(BITDELAY);
		b6 = RxDPort;
		_delay_us(BITDELAY);
		b7 = RxDPort;

	#else
		unsigned char parity;
		_delay_us(FRSTBITDELAY);
		b0 = RxDPort;
		_delay_us(BITDELAY);
		b1 = RxDPort;
		_delay_us(BITDELAY);
		b2 = RxDPort;
		_delay_us(BITDELAY);
		b3 = RxDPort;
		_delay_us(BITDELAY);
		b4 = RxDPort;
		_delay_us(BITDELAY);
		b5 = RxDPort;
		_delay_us(BITDELAY);
		b6 = RxDPort;
		_delay_us(BITDELAY);
		b7 = RxDPort;
		_delay_us(BITDELAY);
		parity = RxDPort;
	#endif
	
	if (PARITY){
		oddeven = (b0&RxDBit)
					 ^ (b1&RxDBit)
					 ^ (b2&RxDBit)
					 ^ (b3&RxDBit)
					 ^ (b4&RxDBit)
					 ^ (b5&RxDBit)
					 ^ (b6&RxDBit)
					 ^ (b7&RxDBit)
					 ^ (parity&RxDBit);
		if(PARITY==2 && oddeven){
			return -1;  // parity was not even
		}
		if(PARITY==1 && !oddeven){
			return -1;  // parity was not odd
		}
	}

	retval = 0;
	if (b0&RxDBit) retval += 1;
	if (b1&RxDBit) retval += 1<<1;
	if (b2&RxDBit) retval += 1<<2;
	if (b3&RxDBit) retval += 1<<3;
	if (b4&RxDBit) retval += 1<<4;
	if (b5&RxDBit) retval += 1<<5;
	if (b6&RxDBit) retval += 1<<6;
	if (b7&RxDBit) retval += 1<<7;

	return retval;
}


#endif