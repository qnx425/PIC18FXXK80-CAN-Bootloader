#ifndef CAN_H_INC
#define CAN_H_INC

#define CAN_CMD (0x450 << 5)    // control  frame
#define CAN_DAT (0x451 << 5)    // data     frame
#define CAN_RSP (0x453 << 5)    // response frame

#define CAN_MSK (0x7FE << 5)    // mask   for CAN_CMD and CAN_DAT
#define CAN_FLT (0x450 << 5)    // filter for CAN_CMD and CAN_DAT

typedef struct {
    unsigned short id;
    unsigned char  dlc; 

    union {                         // Added by KVN
        struct {
            unsigned char           cmd;

            union {
                struct {
                    unsigned long   addr32;     // for PIC18FXXK80
                    unsigned int    extra16;
                };
            
                struct {
                    unsigned int    addr16;     // for PIC16F88X
                    unsigned long   extra32;
                };
            
            };

            unsigned char           len;
        };
        unsigned char               data[8];
    };
} 	canmsg_t;

// function prototypes
extern bit can_send_message		(canmsg_t *);
extern bit can_receive_message	(canmsg_t *);

#endif
