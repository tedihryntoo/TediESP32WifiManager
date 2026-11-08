# TediESP32WiFi

Library pengelola WiFi ESP32 dengan halaman konfigurasi lokal.

## Fitur

- Access Point konfigurasi ESP32.
- Halaman konfigurasi melalui browser.
- Scan jaringan WiFi.
- Mengganti SSID dan password tanpa upload ulang program.
- Penyimpanan konfigurasi menggunakan Preferences.
- Koneksi WiFi Station (STA).
- Reconnect otomatis.
- Access Point tetap aktif ketika STA terhubung.
- Halaman HTML disimpan menggunakan PROGMEM.

## Instalasi

1. Download repository.
2. Buka Arduino IDE.
3. Pilih Sketch > Include Library > Add .ZIP Library.
4. Pilih ZIP TediESP32WiFi.
5. Pilih board ESP32 Dev Module.

## Penggunaan

```cpp
#include <TediESP32WiFi.h>

TediESP32WiFi wifi;

void setup() {
    Serial.begin(115200);
    wifi.begin("TEDI-ESP32", "12345678");
}

void loop() {
    wifi.loop();
}