#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "zg01/zg01_fsm.h"

// happy flow test
static bool test1(void)
{
    uint8_t buffer[5];
    
    memset(buffer, 0xff, 5);
    zg01_init(buffer);
    
    // expect false for the first 39 bits
    bool result;
    for (int i = 0; i < 39; i++) {
        result = zg01_process(i, 0);
        if (result) {
            fprintf(stderr, "expected false for first 39 bits!\n");
            return false;
        }
    }
    
    // expect true for the 40th bit
    result = zg01_process(40, 0);
    if (!result) {
        fprintf(stderr, "expected true for 40th bit!\n");
        return false;
    }
    
    // expect all zeroes in buffer
    uint8_t expected[5] = {0, 0, 0, 0, 0};
    if (memcmp(expected, buffer, 5) != 0) {
        fprintf(stderr, "expected all 0 bytes!\n");
        return false;
    }
    
    return true;
}

// FSM reset by time
static bool test2(void)
{
    uint8_t buffer[5];
    
    zg01_init(buffer);
    
    // send 10 bits, then a gap, then 40 bits
    int ms = 0;
    for (int i = 0; i < 10; i++) {
        zg01_process(ms++, 0);
    }
    
    ms += 10;
    zg01_process(ms++, 1);
    
    bool result;
    for (int i = 0; i < 39; i++) {
        result = zg01_process(ms++, 0);
    }
    if (!result) {
        fprintf(stderr, "expected true after 40 bits after gap!\n");
        return false;
    }
    
    return true;
}

// test that attempts to overflow a buffer
static bool test3(void)
{
    uint8_t buffer[6];

    memset(buffer, 0xFF, sizeof(buffer));
    zg01_init(buffer);
    
    int ms = 100;
    for (int i = 0; i < 1000; i++) {
        zg01_process(ms++, 0);
    }
    
    if (buffer[5] != 0xFF) {
        fprintf(stderr, "detected buffer overflow!\n");
        return false;
    }
    
    return true;
}

static int sendbyte(unsigned long millis, uint8_t b)
{
    unsigned long ms = millis;

    for (int i = 0; i < 8; i++) {
        zg01_process(ms++, (b & (128 >> i)) ? 1 : 0);
    }
    return ms;
}

static bool test4(void)
{
    uint8_t buffer[5];

    memset(buffer, 0, sizeof(buffer));
    zg01_init(buffer);
    unsigned long ms = 0;
    ms = sendbyte(ms, 'B');
    ms = sendbyte(ms, 0x02);
    ms = sendbyte(ms, 0x58);
    ms = sendbyte(ms, 0xAA);
    ms = sendbyte(ms, 0x0D);

    uint8_t expected[] = {0x42, 0x02, 0x58, 0xAA, 0x0D};
    if (memcmp(expected, buffer, 5) != 0) {
        fprintf(stderr, "buffer contents mismatch!\n");
        return false;
    }

    return true;
}

static bool test_ms_overflow(void)
{
    uint8_t buffer[5];

    memset(buffer, 0xAA, sizeof(buffer));
    zg01_init(buffer);
    unsigned long ms = -20;

    for (int i = 0; i < 5; i++) {
        ms = sendbyte(ms, 0x55);
    }

    uint8_t expected[] = {0x55, 0x55, 0x55, 0x55, 0x55};
    if (memcmp(expected, buffer, 5) != 0) {
        fprintf(stderr, "buffer contents mismatch!\n");
        return false;
    }

    return true;
}

// prototype for a test function
typedef bool (testfunc_t)(void);

// prototype for a test case
typedef struct {
    const testfunc_t *fn;
    const char *name;
} test_t;

// list of test cases, each a function and a name
static const test_t testfuncs[] = {
    { test1, "Happy flow" },
    { test2, "Time reset" },
    { test3, "Overflow" },
    { test4, "CO2 message" },
    { test_ms_overflow, "Time overflow" },
    { NULL, ""}
};


// performs test cases, returns 0 if all tests succeeds, returns failing test number otherwise
int main(int argc, char *argv[])
{
    // unused args
    (void)argc;
    (void)argv;    

    int ret = 1;
    for (const test_t *test = testfuncs; test->fn != NULL; test++) {
        printf("Running test %d ('%s') ... ", ret, test->name);
        fflush(stdout);
        bool result = test->fn();
        if (result) {
            printf("OK\n");
        } else {
            return ret;
        }
        ret++;
    }
    
    // all successful
    return 0;
}

