#ifndef __mydefs_h__
#define __mydefs_h__


#define BIT(b)  (0x01<<b)

// m is bit mask, not port number!
#define SETBIT(m)    (PORTB |= (m))
#define CLEARBIT(m)  (PORTB &= ~(m))
#define TOGGLEBIT(m) (PORTB ^= (m))
#define GETBIT(m)    (PORTB & (m))
#define READBIT(m)   (PINB & (m))

#define SETINPUT(m)  (DDRB &= ~(m));
#define SETOUTPUT(m) (DDRB |= (m));

#endif