#include <TediESP32WiFi.h>

TediESP32WiFi wifiManager;

void setup() {
    Serial.begin(115200);

    wifiManager.begin(
        "TEDI-ESP32",
        "12345678"
    );

    Serial.print("Alamat konfigurasi: http://");
    Serial.println(wifiManager.apIP());
}

void printWiFiStatus() {
    Serial.println(wifiManager.connectionStatus());

    if (wifiManager.isConnected()) {
        Serial.print("IP WiFi: ");
        Serial.println(wifiManager.localIP());
    }
}

void loop() {
    wifiManager.loop();

    static unsigned long previousMillis = 0;

    if (millis() - previousMillis >= 5000) {
        previousMillis = millis();
        printWiFiStatus();
    }
}