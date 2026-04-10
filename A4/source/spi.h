#ifndef SPI_HEADER
#define SPI_HEADER

void initSPIPins();
void spiFrequency( unsigned int frequency );
void spiConfig( unsigned int cpol, unsigned int cpha, unsigned int cspol0, unsigned int cspol1, unsigned int cspol );
void spiCESelect( unsigned int ceNumber );
void spiTransfer( unsigned char sendBuff[], unsigned char recvBuff[], unsigned int length );


#endif



