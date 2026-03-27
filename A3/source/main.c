/*
 * Adapted from: https://github.com/bztsrc/raspi3-tutorial/blob/master/07_delays/delays.c
 */

#include "gpio.h"
#include "uart.h"


void printUART(char *str) {
    uart_puts(str);
}


/**
 * Get System Timer's counter
 */
unsigned long get_system_timer()
{
    unsigned int h=-1, l;

    // we must read MMIO area as two separate 32 bit reads
    h = *CHI;
    l = *CLO;

    // we have to repeat it if high word changed during read
    if(h != *CHI) {
        h = *CHI;
        l = *CLO;
    }

    // compose long value
    return ((unsigned long) h << 32) | l;
}

/**
 * Wait N microsec (with BCM System Timer)
 */
void sleep_micro_seconds(unsigned int delay)
{
    unsigned long start_time = get_system_timer();

    // Could theoretically do other things while waiting in this loop
    while (get_system_timer() - start_time < delay);
}


void led_red_on() {
    *GPSET0 = (1 << 4);
}

void led_red_off() {
    *GPCLR0 = (1 << 4);
}


// A function to blink the red LED
void blink() {
    led_red_on();

    // Sleep for 1 second
    sleep_micro_seconds(1000000);

    led_red_off();

    // Sleep for 1 second
    sleep_micro_seconds(1000000);
}


int main() {
    // Configuring for attachment to *Port B*

    printUART("System timer blink demo\n");

    // Initialize the Red LED (GPIO 4) to be an output pin
    *GPFSEL0 = (*GPFSEL0 & ~(0b111 << 12)) | (0b001 << 12);

    // start blinking the LED forever
    while(1) {
        // Blink the LED
        blink();
    }

    return 0;

}

