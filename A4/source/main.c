// 2-gpio-test  main.c - demo the gpio pins


#include <unistd.h>
#include <stdio.h>
#include "uart.h"
#include "timer.h"
#include "spi.h"




// convert unsigned int into character string in base 16
void xtoa( unsigned int n, char *s, int size ) {
    int i;

    // start with null termination - we are interting things backwards
    s[0] = 0;

    // insert the digits starting with least significant
    i = 1;      // start after the null termination
    do {
        int lsdigit = n % 16;
        if ( lsdigit < 10 )
            s[i++] = lsdigit % 10 + '0';
        else
            s[i++] = lsdigit-10 + 'a';
        n /= 16;
    } while( n != 0  && i < size );

    // reverse string
    char temp;
    i--;
    for( int j = 0; j < i; j++ ) {
        temp = s[j];
        s[j] = s[i];
        s[i--] = temp;
    }
}


int itoc( int n, char *s, int i ) {
    if ( n < 0 ) {
        s[i++] = '-';
        i = itoc( -n, s, i );
    } else {
        if ( n >= 10 )
            i = itoc( n / 10, s, i );
        s[i++] = n % 10 + '0';
        s[i] = 0;
    }

    return i;
}





// MCP23S08
#define MCP23S08_IODIR  0x0
#define MCP23S08_IPOL   0x1
#define MCP23S08_GPPU   0x6
#define MCP23S08_GPIO   0x9
#define MCP23S08_OLAT   0xa

#define MCP23S08_READ  0x01
#define MCP23S08_WRITE  0x00
#define MCP23S08_CONTROL  0x40


unsigned char readRegisterMCP23S08( unsigned char reg ) {
    unsigned char spiSendBuffer[32];
    unsigned char spiRecvBuffer[32];

    spiSendBuffer[0] = MCP23S08_CONTROL | MCP23S08_READ;        // control bit pattern for MCP23S08 and set read bit
    spiSendBuffer[1] = reg;                                     // register number
    spiSendBuffer[2] = 0x00;                                    // padding for spi transfer
    spiTransfer( spiSendBuffer, spiRecvBuffer, 3 );
    return spiRecvBuffer[2];
}


void writeRegisterMCP23S08( unsigned char reg, unsigned char value ) {

    // Code missing
    // Exercise: Try implementing yourself!

}


// MCP3008
int mcp3008Convert( unsigned int chan ) {

    // Code missing
    // Exercise: Try implementing yourself!

    return 0;

}



int main() {
    char printBuffer[256];

    uart_puts( "\n----------------------------------------\n\nWelcome\n\n" );

    initSPIPins();
    spiConfig( 0, 0, 0, 0, 0 );
    spiFrequency( 1000000 );
    uart_puts( "SPI init done\n" );

    // set up MCP23S08
    // use CE 1
    spiCESelect( 1 );
    // bits 5:0 for switch input
    writeRegisterMCP23S08( MCP23S08_IODIR, 0x3f );
    // no pull-ups
    writeRegisterMCP23S08( MCP23S08_GPPU, 0x00 );

    while ( 1 ) {
        // set CE for MCP3008 (A/D converter)
        spiCESelect( 0 );
        int v0 = mcp3008Convert( 0 );
        int v1 = mcp3008Convert( 1 );
        int v7 = mcp3008Convert( 7 );

        // set CE for MCP23S08 (GPIO expander)
        spiCESelect( 1 );
        // read GPIO register to get switch states
        int pins = readRegisterMCP23S08( MCP23S08_GPIO );

        // report switch state to uart
        uart_puts( "v0: " );
        itoc( v0, printBuffer, 0 );
        uart_puts( printBuffer );
        uart_puts( "  v1: " );
        itoc( v1, printBuffer, 0 );
        uart_puts( printBuffer );
        uart_puts( " v7: " );
        itoc( v7, printBuffer, 0 );
        uart_puts( printBuffer );

        uart_puts( " buttons: 0x" );
        xtoa( pins & 0x3f, printBuffer, 256 );
        uart_puts( printBuffer );

        uart_puts( "\n" );

        delay( 10 );
    }

    while(1) asm volatile("nop");
}



