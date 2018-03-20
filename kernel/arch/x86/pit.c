
#include <basic.h>
#include <print.h>
#include "portio.h"
#include "pit.h"

#define PIT_CH0 0x40
#define PIT_CH1 0x41
#define PIT_CH2 0x42
#define PIT_CMD 0x43



int set_timer_periodic(int hz) {
    int divisor = 1193182 / hz;

    // 0 represents 65536 and is the largest possible divisor
    // giving 18.2Hz
    if (divisor > 65535)
        divisor = 0;
    
    printf("pit: actual divisor: %i\n", divisor);
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, divisor & 0xFF);   /* Set low byte of divisor */
    outb(PIT_CH0, divisor >> 8);     /* Set high byte of divisor */

    return 0;
}


