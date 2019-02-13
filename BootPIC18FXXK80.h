#ifndef BOOTPIC1825K80_H
#define BOOTPIC1825K80_H

void UserInit(void);
void ProcessIO(void);

#define REMAPPED_APPLICATION_RESET_VECTOR       0x0800
#define BOOTLOADER_ABSOLUTE_ENTRY_ADDRESS       0x001C  //Execute a "goto 0x001C" inline assembly instruction, if you want to enter the bootloader mode from the application via software

#define APP_SIGNATURE_ADDRESS           (REMAPPED_APPLICATION_RESET_VECTOR + 6) //0x1006 and 0x1007 contains the "signature" WORD, indicating successful erase/program/verify operation
#define APP_SIGNATURE_VALUE              0x600D  //leet "GOOD", implying that the erase/program was a success and the bootloader intentionally programmed the APP_SIGNATURE_ADDRESS with this value

#define PROGRAM_MEM_START_ADDRESS       REMAPPED_APPLICATION_RESET_VECTOR //Beginning of application program memory (not occupied by bootloader).  **THIS VALUE MUST BE ALIGNED WITH 64 BYTE BLOCK BOUNDRY** Also, in order to work correctly, make sure the StartPageToErase is set to erase this section.

    #define PROGRAM_MEM_STOP_ADDRESS    _ROMSIZE //**MUST BE WORD ALIGNED (EVEN) ADDRESS.  This address does not get updated, but the one just below it does: IE: If PROGRAM_MEM_STOP_ADDRESS = 0x200, 0x1FF is the last programmed address (0x200 not programmed)**
    #define CONFIG_WORDS_START_ADDRESS  0x300000 //0x300000 is start of CONFIG space for these devices
    #define CONFIG_WORDS_SECTION_LENGTH 14       //14 bytes worth of Configuration words on these devices
    #define USER_ID_ADDRESS             0x200000 //User ID is 8 bytes starting at 0x200000
    #define USER_ID_SIZE                8
    #define DEVICE_WITH_EEPROM                   //Comment this out, if you never want the bootloader to reprogram the EEPROM space
    #define EEPROM_SIZE                 _EEPROMSIZE
    #define EEPROM_EFFECTIVE_ADDRESS    0xF00000 //Location in the .hex file where the EEPROM contents are stored
    #define WRITE_BLOCK_SIZE            _FLASH_WRITE_SIZE
    #define ERASE_PAGE_SIZE             _FLASH_ERASE_SIZE
    #define ERASE_PAGE_ADDRESS_MASK     0xFFFFC0

    #define MAX_PAGE_TO_ERASE           (PROGRAM_MEM_STOP_ADDRESS / ERASE_PAGE_SIZE - 1)    //Last 64 byte page of flash

#define START_PAGE_TO_ERASE             (PROGRAM_MEM_START_ADDRESS/ERASE_PAGE_SIZE)         //The first flash page number to erase, which is the start of the application program space

#define BOOT_MODE                   0x01
#define QUERY_DEVICE                0x02
#define ERASE_DEVICE                0x04
#define PROGRAM_DEVICE              0x05
#define PROGRAM_COMPLETE            0x06
#define GET_DATA					0x07
#define RESET_DEVICE                0x08
#define SIGN_FLASH					0x09

//#define MEMORY_REGION_PROGRAM_MEM   0x01    //When the host sends a QUERY_DEVICE command, need to respond by populating a list of valid memory regions that exist in the device (and should be programmed)
//#define MEMORY_REGION_EEDATA        0x02
//#define MEMORY_REGION_CONFIG        0x03
//#define MEMORY_REGION_USERID        0x04
//#define MEMORY_REGION_END           0xFF    //Sort of serves as a "null terminator" like number, which denotes the end of the memory region list has been reached.
//#define BOOTLOADER_V1_01_OR_NEWER_FLAG   0xA5   //Tacked on in the VersionFlag byte, to indicate when using newer version of bootloader with extended query info available

//#define INVALID_ADDRESS             0xFFFFFFFF
#define INVALID_ADDRESS             0xFFFF
#define CORRECT_UNLOCK_KEY          0xB5

#define BYTES_PER_ADDRESS_PIC18     0x01        //One byte per address.  PIC24 uses 2 bytes for each address in the hex file.
#define WORDSIZE                    0x02    //PIC18 uses 2 byte words, PIC24 uses 3 byte words.
#define REQUEST_DATA_BLOCK_SIZE     8       // KVN

#define FAMILY_PIC18    0x01
#define R_GOOD          0x55
#define R_CHKSUM        0x01
#define R_EEPROM        0x02

#endif
