#include <xc.h>
#include <stdint.h>
#include "can.h"

bit can_send_message(canmsg_t * p_canmsg) {
	unsigned char *p_data, i;

    p_data = (uint8_t *)&TXB2D0;
	for (i = 0; i < 8; i++)
		*p_data++ = p_canmsg->data[i];
    TXB2REQ = 1;

    return 1;
}

bit can_receive_message(canmsg_t * p_canmsg) {
    unsigned char i, *p_data;

	if (RXB0FUL) {
		p_canmsg->id  = ((uint16_t)RXB0SIDH << 8) | (RXB0SIDL & 0xE0);
		p_canmsg->dlc = RXB0DLC & 0x0F;
		p_data = (uint8_t *)&RXB0D0;
		for (i = 0; i < 8; i++)
			p_canmsg->data[i] =  *p_data++;
		RXB0FUL = 0;

        return 1;
    }

    return 0;
}
