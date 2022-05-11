#pragma once
#include "cant_cfg.h"

#define CANT_PERIOD (F_CPU / CANT_BITRATE)
#define CANT_DRIVE_ENABLE() CANT_DE_PORT |= (1 << CANT_DE_PIN)
#define CANT_DRIVE_DISABLE() CANT_DE_PORT &= ~(1 << CANT_DE_PIN)

// bitrate check
#if (!defined(CANT_ACCELERATE)) && CANT_PERIOD < 80
    #error "CANT_BITRATE is too high, consider using CANT_ACCELERATE (if using the internal oscillator), a higher frequency external oscillator or lowering the bitrate"
#elif defined(CANT_ACCELERATE)
    #undef CANT_PERIOD
    #define CANT_PERIOD (F_CPU * 14 / 8 / CANT_BITRATE)
    #if CANT_PERIOD < 80
        #error "CANT_BITRATE is too high even with CANT_ACCELERATE, consider maxing out the internal oscillator frequency"
    #endif
#endif

// drive settings
#define CANT_D_DDR  DDRB
#define CANT_D_PORT PORTB
#ifdef CANT_USE_CMPA
    #define CANT_D_TIMVECT TIMER1_COMPA_vect
    #define CANT_D_PIN     PB1
    #define CANT_D_TCCRA   (1 << COM1A0)
    #define CANT_D_TCCRA_L (1 << COM1A1)
    #define CANT_D_IF      (1 << OCIE1A)
    #define CANT_D_OCR     OCR1A
#elif defined CANT_USE_CMPB
    #define CANT_D_TIMVECT TIMER1_COMPB_vect
    #define CANT_D_PIN     PB2
    #define CANT_D_TCCRA   (1 << COM1B0)
    #define CANT_D_IF      (1 << OCIE1B)
    #define CANT_D_OCR     OCR1B
#else
    #error "Define CANT_USE_CMPA or CANT_USE_CMPB"
#endif

// transmission result
typedef enum {
    CANT_OK = 0,
    CANT_TIMEOUT = 1
} cant_tres_t;

// priority
typedef enum {
    CANT_PRI_NORMAL = 0,
    CANT_PRI_LOW = 1
} cant_pri_t;

// calculates CRC
uint8_t cant_crc(uint8_t* data, uint8_t len);

// initializes CANT
void        cant_init(void);
// transmits a physical frame
cant_tres_t cant_transmit_phy(uint8_t* data, uint8_t len, cant_pri_t pri, uint16_t timeout);
