#include "Arduino.h"
#include <WS2812FX.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

const uint16_t tcp_port = 80;
const char* hostname = "led";
ESP8266WebServer server(tcp_port);

const uint16_t led_count = 24;
const uint8_t led_pin = 4;
WS2812FX leds = WS2812FX(led_count, led_pin, NEO_GRBW + NEO_KHZ800);


void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin();
    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        Serial.print("#");
    }
    Serial.println('\n');
    Serial.print("connected to:\t");
    Serial.println(WiFi.SSID());
    Serial.print("ip address:\t");
    Serial.println(WiFi.localIP());

    if (! MDNS.begin(hostname)) {
        Serial.println("cannot start mdns responder");
    }

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", "led-strip server");
    });

    server.on("/mode", HTTP_POST, []() {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            server.send(400);
            return;
        }

        if (server.method() == HTTP_POST)
        {
            leds.setMode(doc["mode"]);
            server.send(200);
        }
    });

    server.begin();


    leds.init();
    leds.setBrightness(100);
    leds.setSpeed(200);
    leds.setMode(FX_MODE_TWINKLE_FADE_RANDOM);
    leds.start();
}

void loop() {
    server.handleClient();
    MDNS.update();
    leds.service();
}
