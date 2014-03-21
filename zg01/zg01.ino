#include <stdint.h>

#include "zg01_fsm.h"

#define PIN_CLOCK   2
#define PIN_DATA    3

static uint8_t buffer[5];

void setup(void)
{
    // initialize ZG01 pins
    pinMode(PIN_CLOCK, INPUT);
    pinMode(PIN_DATA, INPUT);

    // initialize serial port
    Serial.begin(9600);
    Serial.println("Hello world!\n");
    
    // initialize ZG01 finite state machine
    zg01_init(buffer);
}

void loop(void)
{
    // wait until clock is low
    while (digitalRead(PIN_CLOCK) != LOW);
    
    // sample data and process in the ZG01 state machine
    uint8_t data = (digitalRead(PIN_DATA) == HIGH) ? 1 : 0;
    unsigned long ms = millis();
    bool ready = zg01_process(ms, data);
    
    // process data if ready
    if (ready) {
        for (int i = 0; i < 5; i++) {
            Serial.print(HEX, buffer[i]);
            Serial.print(" ");
        }
        Serial.println();
    }

    // wait until clock is high again
    while (digitalRead(PIN_CLOCK) == LOW);    
}


