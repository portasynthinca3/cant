#pragma once
#include <avr/io.h>

#define CANT_USE_CMPA        // output on PB1
// #define CANT_USE_CMPB        // output on PB2

#define CANT_DE_PORT PORTB
#define CANT_DE_DDR  DDRB
#define CANT_DE_PIN  PB2

#define CANT_R_PORT PORTB
#define CANT_R_DDR  DDRB
#define CANT_R_PIN  PB3

#define CANT_BITRATE 100000L
// #define CANT_ACCELERATE // clock manipulation, read docs first!
