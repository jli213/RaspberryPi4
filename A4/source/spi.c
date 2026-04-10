
#include "spi.h"
#include "gpio.h"




// SPI0 registers and pins
#define SPI0_BASE   0xFE204000


#define SPI0_CS           ((volatile unsigned int*)(SPI0_BASE+0x00000000))
#define SPI0_FIFO         ((volatile unsigned int*)(SPI0_BASE+0x00000004))
#define SPI0_CLK          ((volatile unsigned int*)(SPI0_BASE+0x00000008))
#define SPI0_DLEN         ((volatile unsigned int*)(SPI0_BASE+0x0000000C))
#define SPI0_LTOH         ((volatile unsigned int*)(SPI0_BASE+0x00000010))
#define SPI0_DC           ((volatile unsigned int*)(SPI0_BASE+0x00000014))


// gpio pins for SPI on port A
#define CE1     7
#define CE0     8
#define MISO    9
#define MOSI    10
#define SCLK    11

void initSPIPins() {

    // deactivate pull-ups/dns for SPI pins
    *GPIO_PUP_PDN_CNTRL_REG0 &= ~(0b11 << (2*(MOSI % 16)));      // clear pull-up-down (no resistor)
    *GPIO_PUP_PDN_CNTRL_REG0 &= ~(0b11 << (2*(MISO % 16)));      // clear pull-up-down (no resistor)
    *GPIO_PUP_PDN_CNTRL_REG0 &= ~(0b11 << (2*(CE0  % 16)));      // clear pull-up-down (no resistor)
    *GPIO_PUP_PDN_CNTRL_REG0 &= ~(0b11 << (2*(CE1  % 16)));      // clear pull-up-down (no resistor)
    *GPIO_PUP_PDN_CNTRL_REG0 &= ~(0b11 << (2*(SCLK % 16)));      // clear pull-up-down (no resistor)


    // init SPI pin functions
    *GPFSEL1 &= ~(0b111 << (3*(MOSI % 10)));         // clear function select bits (input)
    *GPFSEL1 |=   0b100 << (3*(MOSI % 10)) ;         // function select bits to ALT0

    *GPFSEL0 &= ~(0b111 << (3*(MISO % 10)));         // clear function select bits (input)
    *GPFSEL0 |=   0b100 << (3*(MISO % 10)) ;         // function select bits to ALT0

    *GPFSEL0 &= ~(0b111 << (3*(CE0  % 10)));         // clear function select bits (input)
    *GPFSEL0 |=   0b100 << (3*(CE0  % 10)) ;         // function select bits to ALT0

    *GPFSEL0 &= ~(0b111 << (3*(CE1  % 10)));         // clear function select bits (input)
    *GPFSEL0 |=   0b100 << (3*(CE1  % 10)) ;         // function select bits to ALT0

    *GPFSEL1 &= ~(0b111 << (3*(SCLK % 10)));         // clear function select bits (input)
    *GPFSEL1 |=   0b100 << (3*(SCLK % 10)) ;         // function select bits to ALT0

}

void spiFrequency( unsigned int frequency ) {
    *SPI0_CLK = ( 250000000 / frequency ) & ~0b1;
}

void spiConfig( unsigned int cpol, unsigned int cpha, unsigned int cspol0, unsigned int cspol1, unsigned int cspol ) {
    *SPI0_CS =  (cpol   <<  3) |    // clock polarity bit
                (cpha   <<  2) |    // clock phase bit
                (cspol <<  6) |     // chip select polarity bit
                (cspol0 << 21) |    // chip select polarity 0 bit
                (cspol1 << 22);     // chip select polarity 1 bit
}



void spiCESelect( unsigned int ceNumber ) {
    // check range of ceNumber
    if ( ceNumber < 0 ) ceNumber = 0;
    if ( ceNumber > 2 ) ceNumber = 2;

    *SPI0_CS &= ~0b11;        // clear chip select bit
    *SPI0_CS |= ceNumber;     // set the chip enable number bits
}




void spiTransfer( unsigned char sendBuff[], unsigned char recvBuff[], unsigned int length ) {

    // clear transmit and receive FIFOs
    *SPI0_CS |= 0b11 << 4;

    // set transfer active bit
    *SPI0_CS |= 1 << 7;

    // send/receive two bytes
    for( int i = 0; i < length; i++ ) {
        // poll TXD bit - while low there is no room to write another byte
        while ( (*SPI0_CS & (1 << 18)) == 0 ) {
            asm volatile( "nop" );
        }
        *SPI0_FIFO = sendBuff[i];

        // poll RXD bit - while low the FIFO is empty, nothing to read
        while ( (*SPI0_CS & (1 << 17)) == 0 ) {
            asm volatile( "nop" );
        }
        recvBuff[i] = *SPI0_FIFO;
    }

    // poll DONE bit until it goes high
    while ( (*SPI0_CS & (1 << 16)) == 0 ) {
        asm volatile( "nop" );
    }

    // clear transfer active bit
    *SPI0_CS &= ~(1 << 7);
}
