#include "zg01_fsm.h"

// max time between ZG01 bits, until a new frame is assumed to have started
#define ZG01_MAX_MS  3

// all of the state of the FSM
typedef struct {
    uint8_t *buffer;
    int num_bits;
    int prev_ms;
} fsm_t;

static fsm_t fsm;
static fsm_t *s = &fsm;

// resets the FSM
static void fsm_reset(void)
{
    s->num_bits = 0;
}

void zg01_init(uint8_t buffer[5])
{
    s->buffer = buffer;
    fsm_reset();
}

bool zg01_process(unsigned long ms, uint8_t data)
{
    // check if a new message has started, based on time since previous bit
    if ((ms - s->prev_ms) > ZG01_MAX_MS) {
        fsm_reset();
    }
    s->prev_ms = ms;
    
    // number of bits received is basically the "state"
    if (s->num_bits < 40) {
        // store it while it fits
        int idx = s->num_bits % 8;
        s->buffer[idx] = (s->buffer[idx] << 1) | data;
        // are we done yet?
        s->num_bits++;
        if (s->num_bits == 40) {
            return true;
        }
    }
    // else do nothing, wait until fsm is reset
    
    return false;
}


