#include <TediESP32WiFi.h>

TediESP32WiFi wifi;

void setup() {
    wifi.begin("TEDI-ESP32", "12345678");
}

void loop() {
    wifi.loop();
}