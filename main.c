#include <stdio.h>                        
#include <stdint.h>                        
#include "BootPIC18FXXK80.h"
#include "config.h"
#include "can.h"

#ifdef  __DEBUG
#define ENABLE_CAN_LED_BLINK_STATUS 
void BlinkCANStatus(void);
#endif
    
void BootMain(void);

void main(void) {
    if (*(const uint16_t *)APP_SIGNATURE_ADDRESS == APP_SIGNATURE_VALUE) {
        #asm
            goto REMAPPED_APPLICATION_RESET_VECTOR
        #endasm
    }
    BootMain();
}

void __at(BOOTLOADER_ABSOLUTE_ENTRY_ADDRESS) BootMain(void) {
    INTCON = 0; 
    STKPTR = 0;  

    // Send 'Response' to Windows PC Bootlader Application  
    // in case firmware makes call "goto 0x1C" 
	TXB2SIDH  = HIGH_BYTE(CAN_RSP);
	TXB2SIDL  =  LOW_BYTE(CAN_RSP);
    TXB2DLC   = 8;
    TXB2D0    = BOOT_MODE;
    TXB2D1    = R_GOOD;

    if ((CANSTAT & 0xE0) == 0) {    // Send in normal mode only
        TXB2REQ  = 1;
        while (TXB2REQ) continue;
    }    
    
    // Configuration mode
    // A request to switch to Configuration mode may not be immediately honored.
    // Module will wait for CAN bus to be idle before switching to Configuration Mode.
    // Request for other modes such as Loopback, Disable etc. may be honored immediately.
    // It is always good practice to wait and verify before continuing.    
    CANCON    = 0x80;							
	while ((CANSTAT & 0xE0) != 0x80) continue;
								
    UserInit();   

    CIOCON    = 0x20;
    LATA      = 0;
#ifdef  __DEBUG
    ANCON0    = 0;                          // Digital port
    ANCON1   &= 0xF8;                       // Digital port
    TRISA    &= 0b11010000;
    // EUSART init
    TRISB6    = 0;
    UxBAUDCON = 0b01001000;
    UxSPBRGH  = BRGVALUE >>  8;
    UxSPBRG   = BRGVALUE & 255;
    UxTXSTA   = 0b00100110;
    UxRCSTA   = 0b10010000;
  
    if (*(const uint16_t *)APP_SIGNATURE_ADDRESS == APP_SIGNATURE_VALUE) 
        signedGood = 1;
#endif
    LATC      = 0x40;                       // RC6 as CANTX output
    TRISC     = 0x80;                       // RC7 as CANRX input

    if (BRGCON1 == 0 && BRGCON2 == 0) {     // reset
        BRGCON1  = BRPVAL;                  // SJW=1
        BRGCON2  = 0b10010001;              // PropSeg=2Tq, PS1=3Tq
        BRGCON3  = 1;                       // PS2=2Tq
    }

    RXM0SIDH  = HIGH_BYTE(CAN_MSK);
    RXM0SIDL  =  LOW_BYTE(CAN_MSK);
    RXM1SIDH  = 0xFF;
    RXM1SIDL  = 0xE0;
	RXF0SIDH  = HIGH_BYTE(CAN_FLT);
	RXF0SIDL  =  LOW_BYTE(CAN_FLT);
	RXF1SIDH  = 0;
	RXF1SIDL  = 0;
	RXF2SIDH  = 0;
	RXF2SIDL  = 0;
	RXF3SIDH  = 0;
	RXF3SIDL  = 0;
	RXF4SIDH  = 0;
	RXF4SIDL  = 0;
	RXF5SIDH  = 0;
	RXF5SIDL  = 0;

    CANCON    = 0;      // CAN normal mode
	while ((CANSTAT & 0xE0) != 0) continue;

    while (1) {
        ClrWdt();
        #ifdef ENABLE_CAN_LED_BLINK_STATUS
            BlinkCANStatus(); 
        #endif        
        ProcessIO();      
    }
}    

#ifdef ENABLE_CAN_LED_BLINK_STATUS
void BlinkCANStatus(void) {
    static uint16_t led_count = 0;
    uint8_t  cmd, i;

    if (--led_count == 0) {
        ledBlink = !ledBlink; 
    }

    if (UxRCIF) {
        cmd = UxRCREG;
        switch (cmd) {
/*
            case 'b':
                 asm("goto 0x7d00");
                 break;
*/
            case 'g':
                 TBLPTR = 0x1030;      
                 for (i = 0; i < 48; i++) {
                    asm("tblrdpostinc");
                    putch(TABLAT);
                 }   
                 TBLPTR = 0x12b0;      
                 for (i = 0; i < 32; i++) {
                    asm("tblrdpostinc");
                    putch(TABLAT);
                 }   
                 TBLPTR = 0x1320;      
                 for (i = 0; i < 96; i++) {
                    asm("tblrdpostinc");
                    putch(TABLAT);
                 }   
                 break;

            default:
                 break;
        }
    }
         
}
#endif

//Compiler mode and version check.  This code needs to fit within the [0x000-0xFFF] program
//memory region that is reserved for use by the bootloader.  However, if this
//code is built in XC8 Standard or Free mode (instead of PRO),
//the code may be too large to fit within the region, and a variety of linker
//error messages (ex: "can't find space") will result.  Unfortunately these
//linker error messages can be cryptic to a user, so instead we add a deliberate
//#error to make a more human friendly error appear, in the event the wrong
//compiler mode is attempted to use to build this code.  If you get this error
//message, please upgrade to the PRO compiler, and then use the mode
//(ex: build configuration --> XC8 compiler --> Option Categories: Optimizations --> Operation Mode: PRO)
#ifdef __XC8__
    #if _HTC_EDITION_ < 2   //Check if PRO, Standard, or Free mode
    #error "This bootloader project must be built in PRO mode to fit within the reserved region.  Double click this message for more details."
    #endif
#endif

/** EOF main.c ***************************************************************/
