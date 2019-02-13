#ifndef __CONFIG_H_INC
#define __CONFIG_H_INC

#include <xc.h>

#define __PLL_ENABLED   1
#define __F_OSC         16000000UL
#define _XTAL_FREQ      (__F_OSC*(1+3*__PLL_ENABLED))

#define UARTNUM         2
#define BAUDRATE        115200UL
#define BRGVALUE        ((_XTAL_FREQ + 2*BAUDRATE) / (4*BAUDRATE) - 1)

#define CANRATE         500000UL
#define TQ_VAL          8
#define BRPVAL          (_XTAL_FREQ / CANRATE / TQ_VAL / 2 - 1)

#if BRPVAL > 63
#error "Baud Rate Prescaler too big."
#endif

#if UARTNUM == 2
    #define UxSPBRG     SPBRG2
    #define UxSPBRGH    SPBRGH2
    #define UxRCSTA     RCSTA2
    #define UxTXSTA     TXSTA2
    #define UxRCREG     RCREG2
    #define UxTXREG     TXREG2
    #define UxPIR       PIR3
    #define UxRCIF      PIR3bits.RC2IF
    #define UxTXIF      PIR3bits.TX2IF
    #define UxRCIE      PIE3bits.RC2IE
    #define UxTXIE      PIE3bits.TX2IE
    #define UxBAUDCON   BAUDCON2
#endif

#define signedGood      LATA0
#define erasing         LATA1
#define badEEPROM       LATA2
#define wrEEPROM        LATA3
#define ledBlink        LATA5

#endif
