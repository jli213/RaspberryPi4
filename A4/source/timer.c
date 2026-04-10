// timer.c - Pi 4 sytem timer functions


/*
#include <unistd.h>
#include <stdio.h>
//#include "gpio.h"
#include "uart.h"
*/
#include "timer.h"

// system timer register defs
#define TIMER_BASE 0xFE003000
#define CS      0
#define CLO     1
#define CHI     2
#define C0      3
#define C1      4
#define C2      5
#define C3      6



// read the 64-bit system timer
unsigned long long getSystemTimerCounter() {
    // from  https://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-System-Timer.html
    volatile unsigned int *timer = (unsigned int *)TIMER_BASE;
    unsigned int h=-1, l;
    
    // we must read MMIO area as two separate 32 bit reads
    h = timer[CHI];
    l = timer[CLO];

    // we have to repeat it if high word changed during read
    //   - low low counter rolled over
    if ( h != timer[CHI] ) {
        h = timer[CHI];
        l = timer[CLO];
     }
    // compose long long int value
    return ((unsigned long long) h << 32) | (unsigned long long)l;
}

// timed delay in us
void delayus( unsigned int delay ) {
    unsigned long long tFuture = getSystemTimerCounter() + delay;
    while ( getSystemTimerCounter() < tFuture );
}

// timed delay in ms
void delay( unsigned int ms ) {
    delayus( 1000 * ms );
}


