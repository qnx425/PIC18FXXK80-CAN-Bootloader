#include "config.h"

#ifdef  __DEBUG

void putch(char data) {
    while (!UxTXIF) continue;
    UxTXREG = data;
}

#endif
