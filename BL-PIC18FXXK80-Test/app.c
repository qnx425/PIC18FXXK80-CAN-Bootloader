#include <xc.h>
#include <stdint.h>

#define FRAME_CONTROL   0x450
#define FRAME_DATA      0x451

__EEPROM_DATA(0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F);
__EEPROM_DATA(0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07);

volatile uint8_t *ptr, cnt = 0;

void interrupt high_priority myIsr(void) {
    if (TMR0IF && TMR0IE) {
        TMR0IF  = 0;
        
        LATA0  ^= 1;
    } 
}

void interrupt low_priority myLoIsr(void) {
    if (TMR1IF && TMR1IE) {
        TMR1IF  = 0;

        TMR1    = 3036;
        
        cnt = (cnt + 1) & 15;
        if (cnt == 0) LATA1  ^= 1;
    }
}

void main(void) {
    uint8_t  len, i;
    uint8_t  buf[8];
    uint32_t UserID = 0;
    
    CANCON    = 0x80;						// Configuration mode
	while ((CANSTAT & 0xE0) != 0x80) continue;

    TBLPTRU = 0x20;
    TBLPTRH = 0x00;
    TBLPTRL = 0x07;

    do {
        asm("tblrdpostdec");
        UserID <<= 4;
        UserID  |= (TABLAT   & 0x0F);
    }   while   (TBLPTRL != 0xFF);
    
    ANCON0   = 0;                           // Digital port
    ANCON1  &= 0xF8;                        // Digital port
    
    LATA     = 0;
    LATB     = 0;
    LATC     = 0x40;                        // RC6 as CANTX output
    TRISA    = 0;
    TRISB    = 0;
    TRISC    = 0x80;                        // RC7 as CANRX input

                                            // 500 kbit/s on 64 MHz
    BRGCON1  = 7;                           // SJW=1, BRP = 8;
    BRGCON2  = 0b10010001;                  // PropSeg=2Tq, PS1=3Tq
    BRGCON3  = 1;                           // PS2=2Tq
    
    RXF0SIDH = LOW_BYTE(FRAME_CONTROL >> 3);
    RXF0SIDL = LOW_BYTE(FRAME_CONTROL << 5);
    RXF1SIDH = LOW_BYTE(FRAME_DATA    >> 3);
    RXF1SIDL = LOW_BYTE(FRAME_DATA    << 5);
    RXM0SIDH = 0xFF;
    RXM1SIDH = 0xFF;
    RXM0SIDL = 0xE0;
    RXM1SIDL = 0xE0;

	TXB2SIDH  = HIGH_BYTE(0x453 << 5);
	TXB2SIDL  =  LOW_BYTE(0x453 << 5);
    TXB2DLC   = 8;
    TXB2D0    = 0x02;
    TXB2D1    = 0x55;

    TMR1     = 3036;
    
    T0CON    = 0x96;                        // 16 bit timer, 1:128 prescaler
    T1CON    = 0x31;                        // 1:8 prescaler
    T3CON    = 0x30;                        // 1:8 prescaler
    TMR3H    = 0;
    TMR3L    = 0;

    CIOCON   = 0x20;
    CANCON   = 0;                           // CAN normal mode
    
    IPEN     = 1;
    TMR1IE   = 1;
    TMR1IP   = 0;
    INTCON   = 0xE0;
    
    while (1) {
        if (TMR3IF) {
            TMR3IF  = 0;
            TMR3ON  = 0;
            TMR3H   = 0;
            TMR3L   = 0;
            LATA   &= 0xF3;
        }
        
        if (RXB0FUL) {
            len = RXB0DLC;
            ptr = &RXB0D0;
            
            TMR3ON = 1;

            if (RXB0FILHIT0) LATA2 = 1;    // Acceptance Filter 1
               else          LATA3 = 1;    // Acceptance Filter 0
            
            len &= 0x0F;
            if (len > 8) len = 8;
            
            if (len)
                for (i = 0; i < len; i++)
                    buf[i] = *ptr++;
            
            RXB0FUL = 0;
            
            if (LATA3) {
                if (buf[0] == 0x01 && *(uint32_t *)&buf[1] == UserID) asm("goto 0x1C");
                if (buf[0] == 0x02) {
                    LATA5  ^= 1;
                    TXB2REQ = 1;
                }
            }
        }
    }
}
