#include "TediESP32WiFi.h"

static const char WIFI_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>TediESP32WiFi</title>

<style>
*{box-sizing:border-box}
body{
    margin:0;
    padding:20px;
    background:#0b1220;
    color:#e5e7eb;
    font-family:Arial,sans-serif
}
.container{
    max-width:480px;
    margin:30px auto;
    padding:22px;
    background:#111c2e;
    border:1px solid #26364f;
    border-radius:14px
}
h2{margin-top:0;color:#60a5fa}
p{color:#9ca3af;font-size:14px}
input,select,button{
    width:100%;
    padding:12px;
    margin:8px 0;
    border-radius:8px;
    border:1px solid #334155;
    font-size:14px
}
input,select{
    background:#0b1220;
    color:white
}
button{
    border:0;
    background:#2563eb;
    color:white;
    font-weight:bold;
    cursor:pointer
}
button.secondary{background:#334155}
button.danger{background:#dc2626}
.status{
    padding:12px;
    background:#0b1220;
    border-radius:8px;
    margin:15px 0;
    font-size:14px
}
small{color:#94a3b8}
</style>
</head>

<body>
<div class="container">
    <h2>TediESP32WiFi</h2>
    <p>Konfigurasi jaringan WiFi ESP32</p>

    <div class="status">
        <div>Status: <strong id="status">Memuat...</strong></div>
        <div>IP ESP32: <strong id="ip">-</strong></div>
        <div>SSID: <strong id="current">-</strong></div>
    </div>

    <label>Pilih jaringan WiFi</label>
    <select id="networks">
        <option value="">Pilih jaringan</option>
    </select>

    <button class="secondary" onclick="scanWiFi()">
        Pindai WiFi
    </button>

    <label>SSID</label>
    <input id="ssid" type="text" placeholder="Nama WiFi">

    <label>Password WiFi</label>
    <input id="password" type="password" placeholder="Password WiFi">

    <button onclick="saveWiFi()">Simpan dan Hubungkan</button>

    <button class="danger" onclick="forgetWiFi()">
        Hapus WiFi Tersimpan
    </button>

    <small>
        Hotspot konfigurasi ESP32 tetap aktif selama perangkat menyala.
    </small>
</div>

<script>
const $ = id => document.getElementById(id);

async function updateStatus(){
    try{
        const response = await fetch('/api/status');
        const data = await response.json();

        $('status').textContent = data.status;
        $('ip').textContent = data.ip;
        $('current').textContent = data.ssid || '-';
    }catch(e){
        $('status').textContent = 'Tidak terhubung';
    }
}

async function scanWiFi(){
    const select = $('networks');
    select.innerHTML = '<option value="">Memindai...</option>';

    try{
        const response = await fetch('/api/scan');
        const data = await response.json();

        select.innerHTML = '<option value="">Pilih jaringan</option>';

        data.networks.forEach(network => {
            const option = document.createElement('option');
            option.value = network.ssid;
            option.textContent =
                network.ssid + ' (' + network.rssi + ' dBm)';
            select.appendChild(option);
        });
    }catch(e){
        select.innerHTML = '<option>Gagal memindai</option>';
    }
}

$('networks').addEventListener('change', function(){
    $('ssid').value = this.value;
});

async function saveWiFi(){
    const ssid = $('ssid').value.trim();
    const password = $('password').value;

    if(!ssid){
        alert('SSID harus diisi');
        return;
    }

    const body = new URLSearchParams();
    body.append('ssid', ssid);
    body.append('password', password);

    try{
        const response = await fetch('/api/save',{
            method:'POST',
            headers:{
                'Content-Type':'application/x-www-form-urlencoded'
            },
            body
        });

        const result = await response.json();
        alert(result.message);
        updateStatus();
    }catch(e){
        alert('Gagal mengirim konfigurasi');
    }
}

async function forgetWiFi(){
    if(!confirm('Hapus konfigurasi WiFi tersimpan?')) return;

    try{
        const response = await fetch('/api/forget',{
            method:'POST'
        });

        const result = await response.json();
        alert(result.message);
        updateStatus();
    }catch(e){
        alert('Gagal menghapus konfigurasi');
    }
}

updateStatus();
setInterval(updateStatus,5000);
</script>
</body>
</html>
)rawliteral";


TediESP32WiFi::TediESP32WiFi()
    : server(80),
      reconnectInterval(15000),
      lastReconnect(0),
      started(false) {
}


void TediESP32WiFi::begin(
    const char* apSSID,
    const char* apPassword
) {
    if(started) return;

    preferences.begin("tedi-wifi", false);

    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");

    WiFi.mode(WIFI_AP_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);

    startAP(apSSID, apPassword);
    startServer();

    if(ssid.length() > 0){
        connectSTA();
    }

    started = true;
}


void TediESP32WiFi::startAP(
    const char* apSSID,
    const char* apPassword
) {
    WiFi.softAP(apSSID, apPassword);
}


void TediESP32WiFi::startServer() {
    server.on("/", HTTP_GET,
        [this]() { handleRoot(); });

    server.on("/api/status", HTTP_GET,
        [this]() { handleStatus(); });

    server.on("/api/scan", HTTP_GET,
        [this]() { handleScan(); });

    server.on("/api/save", HTTP_POST,
        [this]() { handleSave(); });

    server.on("/api/forget", HTTP_POST,
        [this]() { handleForget(); });

    server.onNotFound([this]() {
        server.send(404, "text/plain", "Not Found");
    });

    server.begin();
}


void TediESP32WiFi::connectSTA() {
    if(ssid.length() == 0) return;

    WiFi.begin(ssid.c_str(), password.c_str());
    lastReconnect = millis();
}


void TediESP32WiFi::loop() {
    if(!started) return;

    server.handleClient();

    if(WiFi.status() == WL_CONNECTED){
        return;
    }

    if(ssid.length() == 0) return;

    const unsigned long now = millis();

    if(now - lastReconnect >= reconnectInterval){
        reconnectSTA();
    }
}


void TediESP32WiFi::reconnectSTA() {
    lastReconnect = millis();

    WiFi.disconnect(false, false);
    WiFi.begin(ssid.c_str(), password.c_str());
}


void TediESP32WiFi::handleRoot() {
    server.send_P(
        200,
        "text/html; charset=utf-8",
        WIFI_PAGE
    );
}


String TediESP32WiFi::jsonEscape(const String& value) {
    String result;
    result.reserve(value.length() + 8);

    for(size_t i = 0; i < value.length(); i++){
        const char c = value[i];

        switch(c){
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;

            default:
                if((uint8_t)c >= 0x20){
                    result += c;
                }
                break;
        }
    }

    return result;
}


void TediESP32WiFi::handleStatus() {
    String json;
    json.reserve(180);

    json = "{";

    json += "\"connected\":";
    json += (WiFi.status() == WL_CONNECTED) ? "true" : "false";

    json += ",\"status\":\"";
    json += connectionStatus();

    json += "\",\"ip\":\"";
    json += WiFi.localIP().toString();

    json += "\",\"ssid\":\"";
    json += jsonEscape(connectedSSID());

    json += "\"}";

    server.send(
        200,
        "application/json",
        json
    );
}


void TediESP32WiFi::handleScan() {
    const int count = WiFi.scanNetworks(false, true);

    String json;
    json.reserve(64 + (count > 0 ? count * 80 : 0));

    json = "{\"networks\":[";

    bool first = true;

    for(int i = 0; i < count; i++){
        if(!first) json += ",";

        json += "{\"ssid\":\"";
        json += jsonEscape(WiFi.SSID(i));

        json += "\",\"rssi\":";
        json += String(WiFi.RSSI(i));

        json += ",\"secure\":";
        json += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN)
                    ? "false"
                    : "true";

        json += "}";

        first = false;
    }

    json += "]}";

    WiFi.scanDelete();

    server.send(
        200,
        "application/json",
        json
    );
}


void TediESP32WiFi::handleSave() {
    if(!server.hasArg("ssid")){
        server.send(
            400,
            "application/json",
            "{\"message\":\"SSID tidak ditemukan\"}"
        );
        return;
    }

    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");

    newSSID.trim();

    if(newSSID.length() == 0 || newSSID.length() > 32 ||
       newPassword.length() > 63){
        server.send(
            400,
            "application/json",
            "{\"message\":\"SSID atau password tidak valid\"}"
        );
        return;
    }

    ssid = newSSID;
    password = newPassword;

    preferences.putString("ssid", ssid);
    preferences.putString("password", password);

    WiFi.disconnect(false, false);
    connectSTA();

    server.send(
        200,
        "application/json",
        "{\"message\":\"Konfigurasi tersimpan. ESP32 sedang mencoba terhubung.\"}"
    );
}


void TediESP32WiFi::handleForget() {
    preferences.remove("ssid");
    preferences.remove("password");

    ssid = "";
    password = "";

    WiFi.disconnect(false, false);

    server.send(
        200,
        "application/json",
        "{\"message\":\"Konfigurasi WiFi berhasil dihapus\"}"
    );
}


bool TediESP32WiFi::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}


String TediESP32WiFi::localIP() {
    return WiFi.localIP().toString();
}


String TediESP32WiFi::apIP() {
    return WiFi.softAPIP().toString();
}


String TediESP32WiFi::connectedSSID() {
    if(!isConnected()) return "";
    return WiFi.SSID();
}


String TediESP32WiFi::savedSSID() {
    return ssid;
}


String TediESP32WiFi::connectionStatus() {
    switch(WiFi.status()){
        case WL_CONNECTED:
            return "Terhubung";

        case WL_NO_SSID_AVAIL:
            return "SSID tidak ditemukan";

        case WL_CONNECT_FAILED:
            return "Koneksi gagal";

        case WL_CONNECTION_LOST:
            return "Koneksi terputus";

        case WL_DISCONNECTED:
            return "Tidak terhubung";

        default:
            return "Menghubungkan";
    }
}


void TediESP32WiFi::setReconnectInterval(
    unsigned long interval
) {
    if(interval >= 1000){
        reconnectInterval = interval;
    }
}