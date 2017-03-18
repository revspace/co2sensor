#include <stdint.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#include "zg01_fsm.h"

#define PIN_CLOCK   2
#define PIN_DATA    3

#define MQTT_HOST   "revspace.nl"
#define MQTT_PORT   1883

#define TOPIC_CO2       "revspace/sensors/co2"
#define TOPIC_HUMIDITY  "revspace/sensors/humidity"

static uint8_t buffer[5];
static char esp_id[16];

static WiFiManager wifiManager;
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

void setup(void)
{
    // initialize serial port
    Serial.begin(115200);
    Serial.println("ZG01 ESP reader\n");
    
    // get ESP id
    sprintf(esp_id, "%08X", ESP.getChipId());
    Serial.print("ESP ID: ");
    Serial.println(esp_id);

    // initialize ZG01 pins
    pinMode(PIN_CLOCK, INPUT);
    pinMode(PIN_DATA, INPUT);

    // initialize ZG01 finite state machine
    zg01_init(buffer);

    Serial.println("Starting WIFI manager ...");
    wifiManager.autoConnect("ESP-ZG01");
}

static void mqtt_send(const char *topic, int value, const char *unit)
{
    if (!mqttClient.connected()) {
        mqttClient.setServer(MQTT_HOST, MQTT_PORT);
        mqttClient.connect(esp_id);
    }
    if (mqttClient.connected()) {
        char string[64];
        snprintf(string, sizeof(string), "%d %s", value, unit);
        Serial.print("Publishing ");
        Serial.print(value);
        Serial.print(" to ");
        Serial.print(topic);
        Serial.print("...");
        int result = mqttClient.publish(topic, string, true);
        Serial.println(result ? "OK" : "FAIL");
    }    
}

static void hexdump(const char *buf, int len)
{
    char tmp[8];
    int i;
    for (i = 0; i < sizeof(buffer); i++) {
        snprintf(tmp, sizeof(tmp), " %02X", buffer[i]);
        Serial.print(tmp); 
    }
    Serial.println();
}

void loop(void)
{
    static bool prev_clk = false;
    
    bool clk = (digitalRead(PIN_CLOCK) == HIGH);
    if (prev_clk && !clk) {
        // falling edge, sample data
        bool data = (digitalRead(PIN_DATA) == HIGH); 

        // feed it to the state machine
        unsigned long ms = millis();
        bool ready = zg01_process(ms, data);
        if (ready) {
            // dump raw buffer
            Serial.print("RAW:");
            hexdump(buffer, sizeof(buffer));
        
            // get item and binary value
            uint8_t item = buffer[0];
            uint16_t value = buffer[1] << 8 | buffer[2];
            
            switch (item) {
            case 'P':
                // CO2
                mqtt_send(TOPIC_CO2, value, "ppm");
                break;
            case 'A':
                // humidity
                mqtt_send(TOPIC_HUMIDITY, value, "%");
                break;
            default:
                // ignore unhandled packet
                break;
            }
        }
    }
    prev_clk = clk;
}

