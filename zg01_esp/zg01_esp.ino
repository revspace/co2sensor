#include <stdint.h>

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

#include <Arduino.h>
#include <ArduinoOTA.h>

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

    // setup OTA
    ArduinoOTA.setHostname("esp-co2sensor");
    ArduinoOTA.setPassword("co2sensor");
    ArduinoOTA.begin();

    // initialize ZG01 pins
    pinMode(PIN_CLOCK, INPUT);
    pinMode(PIN_DATA, INPUT);

    // initialize ZG01 finite state machine
    zg01_init(buffer);

    Serial.println("Starting WIFI manager ...");
    wifiManager.setConfigPortalTimeout(120);
    wifiManager.autoConnect("ESP-ZG01");
}

static bool mqtt_send(const char *topic, const char *value, bool retained)
{
    bool result = false;
    if (!mqttClient.connected()) {
        Serial.print("Connecting MQTT...");
        mqttClient.setServer(MQTT_HOST, MQTT_PORT);
        result = mqttClient.connect(esp_id, topic, 0, retained, "offline");
        Serial.println(result ? "OK" : "FAIL");
    }
    if (mqttClient.connected()) {
        Serial.print("Publishing ");
        Serial.print(value);
        Serial.print(" to ");
        Serial.print(topic);
        Serial.print("...");
        result = mqttClient.publish(topic, value, retained);
        Serial.println(result ? "OK" : "FAIL");
    }
    return result;
}

void loop(void)
{
    static bool prev_clk = false;
    char valstr[16];
    int temp10;
    
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
                mqtt_send(TOPIC_CO2, valstr, true);
                break;
            case 'A':
                // humidity
                snprintf(valstr, sizeof(valstr), "%d.%02d %%", value / 100, value % 100);
                mqtt_send(TOPIC_HUMIDITY, valstr, true);
                break;
            case 'B':
                // temperature
                temp10 = (5 * value - 21848) / 8;
                snprintf(valstr, sizeof(valstr), "%d.%d °C", temp10 / 10, abs(temp10) % 10);
                mqtt_send(TOPIC_TEMPERATURE, valstr, true);
                break;
            default:
                // ignore unhandled packet
                break;
            }
        }
    }
    prev_clk = clk;

    // keep mqtt alive
    mqttClient.loop();

    // allow OTA
    ArduinoOTA.handle();

    // verify network connection and reboot on failure
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Restarting ESP...");
        ESP.restart();
    }
}

