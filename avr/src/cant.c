#include "cant.h"
#include "cant_cfg.h"
#include <util/delay.h>
#include <util/crc16.h>
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(CANT_D_TIMVECT) {

}

uint8_t cant_crc(uint8_t* data, uint8_t len) {
   uint8_t crc = 0;
   for(int i = 0; i < len; i++) {
      uint8_t byte = data[i];
      for(char j = 8; j > 0; j--) {
         uint8_t sum = (crc ^ byte) & 1;
         crc >>= 1;
         if (sum)
            crc ^= 0x8C;
         byte >>= 1;
      }
   }
   return crc;
}

void cant_init(void) {
    // drive
    CANT_D_DDR |= (1 << CANT_D_PIN);
    CANT_D_PORT &= ~(1 << CANT_D_PIN);
    // drive enable
    CANT_DE_DDR |= (1 << CANT_DE_PIN);
    CANT_DE_PORT &= ~(1 << CANT_DE_PIN);
    // read
    CANT_R_DDR &= ~(1 << CANT_R_PIN);
    CANT_R_PORT &= ~(1 << CANT_R_PIN);
}

cant_tres_t cant_transmit_phy(uint8_t* data, uint8_t len, cant_pri_t pri, uint16_t timeout) {
    // calculate CRC
    uint8_t crc = cant_crc(data, len);

    // TODO: wait for line to become clear

    // accelerate!!!!
    #ifdef CANT_ACCELERATE
    uint8_t osccal = OSCCAL;
    OSCCAL = 0xFF;
    #endif

    // drive enable
    CANT_DRIVE_ENABLE();

    TCCR1A = CANT_D_TCCRA; // comparator channel A/B, toggle on match
    TCCR1B = (1 << WGM12) | (1 << CS10); // CTC, /1 prescaler
    TCNT1 = 0;
    OCR1A = 65535;
    cli();

    // waits N bit times and toggles the output
    #define start_toggle_after(n) {\
        TIFR1 |= CANT_D_IF;\
        CANT_D_OCR = (CANT_PERIOD * (n)) - 1;\
    }
    #define finish_toggle() {\
        while(!(TIFR1 & CANT_D_IF));\
        TIFR1 |= CANT_D_IF;\
    }

    // sync
    start_toggle_after(1);
    finish_toggle();
    start_toggle_after(1);

    uint8_t zeros = 0;
    for(uint8_t byte = 0; byte < len + 1; byte++) {
        uint8_t byte_val = (byte == len) ? crc : data[byte];

        // transmit from MSB to LSB
        for(uint8_t bit = 0; bit < 8; bit++) {
            // get MSB and shift left
            uint8_t bit_val = byte_val & 0x80;
            byte_val <<= 1;

            // stuffing
            if(zeros == 4) {
                finish_toggle();
                start_toggle_after(5);
                zeros = 0;
            }

            if(bit_val) {
                finish_toggle();
                start_toggle_after(zeros + 1);
                zeros = 0;
            } else {
                zeros++;
            }
        }
    }

    // wait 6 bit times
    finish_toggle();
    TCCR1A = CANT_D_TCCRA_L; // low on match
    start_toggle_after(6);
    finish_toggle();

    // release
    TCCR1B = 0;
    TIMSK1 &= ~CANT_D_IF; // disable comparator interrupt
    CANT_D_PORT &= ~(1 << CANT_D_PIN);
    CANT_DRIVE_DISABLE();

    #ifdef CANT_ACCELERATE
    OSCCAL = osccal;
    #endif

    #undef start_toggle_after
    #undef finish_toggle
    return CANT_OK;
}
