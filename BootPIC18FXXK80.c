#include <stdio.h>
#include <stdint.h>
#include "BootPIC18FXXK80.h"
#include "config.h"
#include "can.h"

void SignFlash          (void);
void WriteEEPROM        (void);
void WriteFlashBlock    (void);
void UnlockAndActivate  (uint8_t UnlockKey);
void erase_device       (void);

static uint32_t     chipID;

static canmsg_t     rx, tx;
static uint8_t      ProgrammingBuffer[WRITE_BLOCK_SIZE], tmpBuffer[WRITE_BLOCK_SIZE];
static uint16_t     ProgrammedPointer, eepromPointer;
static uint8_t      OffsetTail, OffsetHead;

static union {
    char  c[2];
    int   d;
}   devID;

void UserInit(void) {
    TBLPTR = 0x3FFFFE;
    asm("tblrdpostinc");
    devID.c[0] = TABLAT & 0xE0;
    asm("tblrdpostinc");
    devID.c[1] = TABLAT;

    TBLPTR = 0x200007;
    do {
        asm("tblrdpostdec");
        chipID <<= 4;
        chipID  |= (TABLAT   & 0x0F);
    }   while   (TBLPTRL != 0xFF);
    
    TBLPTRU = 0;
}
  
void ProcessIO(void) {
    uint8_t  i;
    
    if (can_receive_message(&rx)) {
        if (rx.id == CAN_DAT) {
            if (!wrEEPROM) {
                for (i = 0; i < 8; i++)
                     tmpBuffer[OffsetTail++] = rx.data[i];

                if (OffsetTail == WRITE_BLOCK_SIZE) {
                    WriteFlashBlock();
                    OffsetTail  = 0;
                    OffsetHead  = 0;
                    ProgrammedPointer += WRITE_BLOCK_SIZE;
                }
            }
            else WriteEEPROM();
        }
        else {
            switch (rx.cmd) {
                case BOOT_MODE:        
                     tx.data[0] = BOOT_MODE; 
                     tx.data[1] = R_CHKSUM; 
                     if (rx.addr32 == chipID) 
                        tx.data[1] = R_GOOD; 
                     break;

                case QUERY_DEVICE:     
                     tx.data[0] = QUERY_DEVICE; 
                     tx.data[1] = devID.c[1]; 
                     tx.data[2] = devID.c[0]; 
                     break;

                case ERASE_DEVICE:     
                     tx.data[0] = ERASE_DEVICE; 
                     erase_device();
                     break;

                case PROGRAM_DEVICE:   
                     tx.data[0] = PROGRAM_DEVICE; 
                     if (rx.data[3] == 0xF0) {
                         wrEEPROM = 1;
                         eepromPointer = rx.addr16 & (EEPROM_SIZE-1);
                     }
                     else if (rx.data[3] != 0x20 && rx.data[3] != 0x30) {
                         ProgrammedPointer  = rx.addr16 & (0xFFFF ^ (WRITE_BLOCK_SIZE - 1));
                         OffsetHead         = rx.addr16 - ProgrammedPointer;
                         OffsetTail         = OffsetHead;
                     }
                     break;

                case PROGRAM_COMPLETE: 
                     tx.data[0] = PROGRAM_COMPLETE;  
                     WriteFlashBlock();
                     wrEEPROM = 0;
                     break;

                case GET_DATA:     
                     if (rx.data[3] == 0xF0) {  // DATA EEPROM READ
                        for (i = 0; i < 8; i++) {
                            _LOAD_EEADR(rx.addr16 + i);
                            EECON1 = 0;         //EEPROM Read mode
                            RD = 1;
                            NOP();
                            tx.data[i]= EEDATA; 
                        }
                     }
                     else {                     // PROGRAM MEMORY READ
                        TBLPTR = rx.addr32;
                        for (i = 0; i < 8; i++) {
                            asm("tblrdpostinc");
                            tx.data[i] = TABLAT;
                        }   
                     }
                     break;

                case RESET_DEVICE:     
                     tx.data[0] = RESET_DEVICE;      
                     break;

                case SIGN_FLASH:     
                     tx.data[0] = SIGN_FLASH;
                     SignFlash();      
                     break;

                default:               
                     tx.data[0] = 0;                 
                     break;
            }
        }

        // do not send response if chipID does not match received value
        if (!(rx.id == CAN_CMD && rx.cmd == BOOT_MODE && tx.data[1] != R_GOOD)) {
            can_send_message(&tx);
        }
        
        if (rx.id == CAN_CMD && rx.cmd == RESET_DEVICE) { 
            __delay_ms(120);
            Reset();    
        }
    }
}

//Should be called once, only after the regular erase/program/verify sequence 
//has completed successfully.  This function will program the magic
//APP_SIGNATURE_VALUE into the magic APP_SIGNATURE_ADDRESS in the application
//flash memory space.  This is used on the next bootup to know that the the
//flash memory image of the application is intact, and can be executed.
//This is useful for recovery purposes, in the event that an unexpected
//failure occurs during the erase/program sequence (ex: power loss or user
//unplugging the USB cable).
void SignFlash(void)
{
    static unsigned char i;
    
    //First read in the erase page contents of the page with the signature WORD
    //in it, and temporarily store it in a RAM buffer.
    TBLPTR = (APP_SIGNATURE_ADDRESS & ERASE_PAGE_ADDRESS_MASK);
    for(i = 0; i < ERASE_PAGE_SIZE; i++)
    {
        asm("tblrdpostinc");  
        ProgrammingBuffer[i] = TABLAT;        
    }    
    
    //Now change the signature WORD value at the correct address in the RAM buffer
    ProgrammingBuffer[(APP_SIGNATURE_ADDRESS & ~ERASE_PAGE_ADDRESS_MASK)] = (unsigned char)APP_SIGNATURE_VALUE;
    ProgrammingBuffer[(APP_SIGNATURE_ADDRESS & ~ERASE_PAGE_ADDRESS_MASK) + 1] = (unsigned char)(APP_SIGNATURE_VALUE >> 8);
    
    //Now erase the flash memory block with the signature WORD in it
    TBLPTR = APP_SIGNATURE_ADDRESS & ERASE_PAGE_ADDRESS_MASK;
    EECON1 = 0x94;  //Prepare for flash erase operation
    UnlockAndActivate(CORRECT_UNLOCK_KEY);
    
    //Now re-program the values from the RAM buffer into the flash memory.  Use
    //reverse order, so we program the larger addresses first.  This way, the 
    //write page with the flash signature word is the last page that gets 
    //programmed (assuming the flash signature resides on the lowest address
    //write page, which is recommended, so that it becomes the first page
    //erased, and the last page programmed).
    TBLPTR = (APP_SIGNATURE_ADDRESS & ERASE_PAGE_ADDRESS_MASK) + ERASE_PAGE_SIZE - 1;   //Point to last byte on the erase page
    i = ERASE_PAGE_SIZE - 1;
    while(1)
    {
        TABLAT = ProgrammingBuffer[i];
        asm("tblwt");
        
        //Check if we are at a program write block size boundary
        if((i % WRITE_BLOCK_SIZE) == 0)
        {
            //The write latches are full, time to program the block.
            ClrWdt();
            EECON1 = 0x84;  //Write to flash on next WR = 1 operation
            UnlockAndActivate(CORRECT_UNLOCK_KEY);
        }    
        asm("tblrdpostdec");
        
        //Check if we are done writing all blocks
        if(i == 0)
        {
            break;  //Exit while loop
        }    
        i--;
    }    
}    

void WriteFlashBlock(void) {
    uint8_t i;
    
    // 1. Read the 64 bytes into RAM.
    TBLPTRU = 0x00;
    TBLPTRH = HIGH_BYTE(ProgrammedPointer);
    TBLPTRL =  LOW_BYTE(ProgrammedPointer);
    for (i = 0; i < WRITE_BLOCK_SIZE; i++) {
        asm("tblrdpostinc");  
        ProgrammingBuffer[i] = TABLAT;        
    }    
    
    // 2. Update the data values in RAM as necessary.
    for (i = OffsetHead; i < OffsetTail; i++) {
        ProgrammingBuffer[i] = tmpBuffer[i];
    }
    
    // 3. Load the Table Pointer register with the address being erased.
    TBLPTRU = 0x00;
    TBLPTRH = HIGH_BYTE(ProgrammedPointer);
    TBLPTRL =  LOW_BYTE(ProgrammedPointer);
    EECON1  = 0b10010100;    //Prepare for erasing flash memory
    
    // 4. Execute the row erase procedure.
    UnlockAndActivate(CORRECT_UNLOCK_KEY);
    asm("tblrdpostdec");    // see WRITING TO FLASH PROGRAM MEMORY EXAMPLE in datasheet
    
    // 5. Load the Table Pointer register with the address of the first byte being written.
    TBLPTRU = 0x00;
    TBLPTRH = HIGH_BYTE(ProgrammedPointer);
    TBLPTRL =  LOW_BYTE(ProgrammedPointer);
    //TBLPTRL =  LOW_BYTE(ProgrammedPointer + OffsetHead);

    // 6. Write the 64 bytes into the holding registers with auto-increment.
    for (i = 0; i < WRITE_BLOCK_SIZE; i++) {
        TABLAT = ProgrammingBuffer[i];
        asm("tblwtpostinc");
    }    

    // Need to make table pointer point to the region which will be programmed before initiating the programming operation
    // mandatory!!!
    // Do this instead of TBLPTR--; since it takes less code space.
    asm("tblrdpostdec");
    
    // 7. Execute the write procedure.
    EECON1 = 0b10000100;    //flash programming mode
    UnlockAndActivate(CORRECT_UNLOCK_KEY);
}

void WriteEEPROM(void) {
     uint8_t i;

     for(i = 0; i < 8; i++) {
        _LOAD_EEADR(eepromPointer + i);

        EEDATA = rx.data[i];

        EECON1 = 0b00000100;    //EEPROM Write mode
        UnlockAndActivate(CORRECT_UNLOCK_KEY);
        RD = 1;
        NOP();
        if (rx.data[i] != EEDATA) 
            badEEPROM = 1;
     }
     
     eepromPointer += 8;
}

//It is preferrable to only place this sequence in only one place in the flash memory.
//This reduces the probabilty of the code getting executed inadvertently by
//errant code.  It is also recommended to enable BOR (in hardware) and/or add
//software checks to avoid microcontroller "overclocking".  Always make sure
//to obey the voltage versus frequency graph in the datasheet, even during
//momentary events (such as the power up and power down ramp of the microcontroller).
void UnlockAndActivate(unsigned char UnlockKey)
{

    INTCONbits.GIE = 0;     //Make certain interrupts disabled for unlock process.

    //Make sure voltage is sufficient for safe self erase/write operations
//    LowVoltageCheck();

    //Check to make sure the caller really was trying to call this function.
    //If they were, they should always pass us the CORRECT_UNLOCK_KEY.
    if(UnlockKey != CORRECT_UNLOCK_KEY)
    {
        //Warning!  Errant code execution detected.  Somehow this 
        //UnlockAndActivate() function got called by someone that wasn't trying
        //to actually perform an NVM erase or write.  This could happen due to
        //microcontroller overclocking (or undervolting for an otherwise allowed
        //CPU frequency), or due to buggy code (ex: incorrect use of function 
        //pointers, etc.).  In either case, we should execute some fail safe 
        //code here to prevent corruption of the NVM contents.
        OSCCON = 0x03;  //Switch to INTOSC at low frequency
        while(1)
        {
            Sleep();
        }    
        Reset();
    }    

    #ifdef __XC8__
        EECON2 = 0x55;
        EECON2 = 0xAA;
        EECON1bits.WR = 1;
    #else
        _asm
        //Now unlock sequence to set WR (make sure interrupts are disabled before executing this)
        MOVLW 0x55
        MOVWF EECON2, 0
        MOVLW 0xAA
        MOVWF EECON2, 0
        BSF EECON1, 1, 0        //Performs write by setting WR bit
        _endasm
    #endif

    while(EECON1bits.WR);   //Wait until complete (relevant when programming EEPROM, not important when programming flash since processor stalls during flash program)  
    EECON1bits.WREN = 0;    //Good practice now to clear the WREN bit, as further protection against any accidental activation of self write/erase operations.
}   

void erase_device(void) {
    unsigned int i;
#ifdef  __DEBUG
    erasing = 1;
#endif
    //First erase main program flash memory
    for (i = START_PAGE_TO_ERASE; i < (MAX_PAGE_TO_ERASE + 1); i++) {
        ClrWdt();
        #ifdef __XC8__
           TBLPTRU = 0x00;
           TBLPTRH = (uint8_t)(i >> 2);
           TBLPTRL = (uint8_t)(i << 6);
        #else
           TBLPTR = i << 6;
        #endif
        EECON1 = 0b10010100;    //Prepare for erasing flash memory
        UnlockAndActivate(CORRECT_UNLOCK_KEY);
        //asm("tblrdpostdec");    // see WRITING TO FLASH PROGRAM MEMORY EXAMPLE in datasheet
    }

    //Now erase EEPROM (if any is present on the device)
    badEEPROM = 0;
    for (i = 0; i < EEPROM_SIZE; i++) {
        _LOAD_EEADR(i);

        EEDATA = 0xFF;
        EECON1 = 0b00000100;    //EEPROM Write mode
        UnlockAndActivate(CORRECT_UNLOCK_KEY);     
        RD = 1;
        NOP();
        if (0xFF != EEDATA) badEEPROM = 1;
    }
#ifdef  __DEBUG
    erasing = 0;
#endif
    //Now erase the User ID space (0x200000 to 0x200007)
//    TBLPTRU = 0x20;
//    TBLPTRH = 0x00;
//    TBLPTRL = 0x00;
//    EECON1 = 0b10010100;    //Prepare for erasing flash memory
//    UnlockAndActivate(CORRECT_UNLOCK_KEY);
}
