#include "wifi_config.h"
#include <WiFi.h>

bool wifi_connect() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.print("正在连接WiFi: ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi连接成功! IP地址: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println();
        Serial.println("WiFi连接失败!");
        return false;
    }
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

