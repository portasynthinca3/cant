#include <avr/io.h>
#include "cant.h"
#include <string.h>

int main() {
    cant_init();

    char* str = "Hello, World!";
    cant_transmit_phy(str, strlen(str), CANT_PRI_LOW, 1000);

    while(1);
}
