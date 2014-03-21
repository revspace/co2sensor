#include <stdint.h>
#include <stdbool.h>

// call this before calling zg01_process
void zg01_init(uint8_t buffer[5]);

// call this for each falling edge of the clock, returns true if a full frame was received
bool zg01_process(unsigned long ms, uint8_t data);

