#ifndef TEDI_ESP32_WIFI_H
#define TEDI_ESP32_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

class TediESP32WiFi {
public:
    TediESP32WiFi();

    void begin(
        const char* apSSID = "ESP32-CONFIG",
        const char* apPassword = "12345678"
    );

    void loop();

    bool isConnected();
    String localIP();
    String apIP();
    String connectedSSID();
    String savedSSID();
    String connectionStatus();

    void setReconnectInterval(unsigned long interval);

private:
    WebServer server;
    Preferences preferences;

    String ssid;
    String password;

    unsigned long reconnectInterval;
    unsigned long lastReconnect;

    bool started;

    void startAP();
    void startServer();
    void connectSTA();
    void reconnectSTA();

    void handleRoot();
    void handleStatus();
    void handleScan();
    void handleSave();
    void handleForget();

    String jsonEscape(const String& value);
};

#endif