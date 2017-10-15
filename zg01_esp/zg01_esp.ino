#include <stdint.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#include "zg01_fsm.h"

#define PIN_CLOCK   D1
#define PIN_DATA    D2

#define MQTT_HOST   "mosquitto.space.revspace.nl"
#define MQTT_PORT   1883

#define TOPIC_CO2       "revspace/sensors/co2"
#define TOPIC_HUMIDITY  "revspace/sensors/humidity"
#define TOPIC_TEMPERATURE "revspace/sensors/temperature"

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

static void mqtt_send(const char *topic, const char *value)
{
    if (!mqttClient.connected()) {
        mqttClient.setServer(MQTT_HOST, MQTT_PORT);
        mqttClient.connect(esp_id);
    }
    if (mqttClient.connected()) {
        Serial.print("Publishing ");
        Serial.print(value);
        Serial.print(" to ");
        Serial.print(topic);
        Serial.print("...");
        int result = mqttClient.publish(topic, value, true);
        Serial.println(result ? "OK" : "FAIL");
    }    
}

static char *frac16(int value)
{
    int frac = abs(value) % 16;
    switch (frac) {
    default:
    case 0:     return ".0000";
    case 1:     return ".0625";
    case 2:     return ".1250";
    case 3:     return ".1875";
    case 4:     return ".2500";
    case 5:     return ".3125";
    case 6:     return ".3750";
    case 7:     return ".4375";
    case 8:     return ".5000";
    case 9:     return ".5625";
    case 10:    return ".6250";
    case 11:    return ".6875";
    case 12:    return ".7500";
    case 13:    return ".8125";
    case 14:    return ".8750";
    case 15:    return ".9375";
    }
}

void loop(void)
{
    static bool prev_clk = false;
    char valstr[16];
    int temp16;
    
    bool clk = (digitalRead(PIN_CLOCK) == HIGH);
    if (prev_clk && !clk) {
        // falling edge, sample data
        bool data = (digitalRead(PIN_DATA) == HIGH); 

        // feed it to the state machine
        unsigned long ms = millis();
        bool ready = zg01_process(ms, data);
        if (ready) {
            // get item and binary value
            uint8_t item = buffer[0];
            uint16_t value = buffer[1] << 8 | buffer[2];
            
            switch (item) {
            case 'P':
                // CO2
                snprintf(valstr, sizeof(valstr), "%d PPM", value);
                mqtt_send(TOPIC_CO2, valstr);
                break;
            case 'A':
                // humidity
                snprintf(valstr, sizeof(valstr), "%d.%02d %%", value / 100, value % 100);
                mqtt_send(TOPIC_HUMIDITY, valstr);
                break;
            case 'B':
                // temperature
                temp16 = value - 4370;
                snprintf(valstr, sizeof(valstr), "%d.%s °C", temp16 / 16, frac16(temp16));
                mqtt_send(TOPIC_TEMPERATURE, valstr);
                break;
            default:
                // ignore unhandled packet
                break;
            }
        }
    }
    prev_clk = clk;
}

