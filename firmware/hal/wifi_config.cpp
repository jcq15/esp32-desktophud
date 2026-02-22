#include "wifi_config.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESP.h>  // 用于ESP.restart()
#include "../core/data_hub.h"
#include "../core/scheduler.h"

// 辅助函数：更新状态消息
void update_wifi_status(Scheduler* scheduler, const String& message) {
    if (scheduler) {
        scheduler->updateStatusMessage(message);
    } else {
        // 如果没有scheduler，直接更新全局dataHub
        extern DataHub dataHub;
        dataHub.statusMessage = message;
    }
}

// 尝试连接已保存的WiFi（通过WiFiManager）
bool try_saved_wifi(Scheduler* scheduler) {
    WiFiManager wm;
    
    // 检查是否有已保存的WiFi配置
    String savedSSID = wm.getWiFiSSID(true);  // true表示从NVS读取
    String savedPass = wm.getWiFiPass(true);
    
    if (savedSSID.length() == 0) {
        Serial.println("No saved WiFi config");
        return false;
    }
    
    Serial.print("Trying saved WiFi: ");
    Serial.println(savedSSID);
    
    String statusMsg = "Connecting to " + savedSSID + "...";
    update_wifi_status(scheduler, statusMsg);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected! IP: ");
        Serial.println(WiFi.localIP());
        update_wifi_status(scheduler, "WiFi connected");
        return true;
    } else {
        Serial.println();
        Serial.println("Saved WiFi connection failed");
        WiFi.disconnect();
        return false;
    }
}

// 尝试连接配置文件中的WiFi
bool try_config_wifis(Scheduler* scheduler) {
    WiFi.mode(WIFI_STA);
    
    // 依次尝试所有WiFi配置
    for (int i = 0; i < WIFI_CONFIG_COUNT; i++) {
        Serial.print("Trying WiFi [");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(WIFI_CONFIG_COUNT);
        Serial.print("]: ");
        Serial.println(WIFI_CONFIGS[i].ssid);

        String statusMsg = "Connecting to " + String(WIFI_CONFIGS[i].ssid) + " [" + String(i + 1) + "/" + String(WIFI_CONFIG_COUNT) + "]...";
        update_wifi_status(scheduler, statusMsg);

        WiFi.begin(WIFI_CONFIGS[i].ssid, WIFI_CONFIGS[i].password);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.print("Connected! SSID: ");
            Serial.print(WIFI_CONFIGS[i].ssid);
            Serial.print(", IP: ");
            Serial.println(WiFi.localIP());
            update_wifi_status(scheduler, "WiFi connected");
            return true;
        } else {
            Serial.println();
            Serial.println("Failed, trying next...");
            WiFi.disconnect();
            delay(500);  // 短暂延迟后尝试下一个
        }
    }

    Serial.println("All config WiFi failed!");
    return false;
}

// 开启WiFi配置门户
bool start_config_portal(Scheduler* scheduler) {
    Serial.println("Starting WiFi config portal...");
    Serial.print("Connect to AP: ");
    Serial.println(WIFI_MANAGER_AP_NAME);
    if (strlen(WIFI_MANAGER_AP_PASSWORD) > 0) {
        Serial.print("Password: ");
        Serial.println(WIFI_MANAGER_AP_PASSWORD);
    }
    Serial.println("Open browser: 192.168.4.1");
    
    String statusMsg = "WiFi setup: Connect to " + String(WIFI_MANAGER_AP_NAME);
    update_wifi_status(scheduler, statusMsg);
    
    WiFiManager wm;
    wm.setConfigPortalTimeout(WIFI_MANAGER_TIMEOUT_SEC);
    
    // 开启配置门户，等待用户配置
    bool connected = wm.autoConnect(WIFI_MANAGER_AP_NAME, WIFI_MANAGER_AP_PASSWORD);
    
    if (connected) {
        Serial.print("Connected via portal! IP: ");
        Serial.println(WiFi.localIP());
        update_wifi_status(scheduler, "WiFi connected");
        return true;
    } else {
        Serial.println("Config portal timeout, restarting...");
        update_wifi_status(scheduler, "WiFi setup timeout");
        delay(2000);
        ESP.restart();
        return false;  // 不会执行到这里
    }
}

bool wifi_connect(Scheduler* scheduler) {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    // 步骤1: 先尝试使用已保存的WiFi连接
    Serial.println("Step 1: Trying saved WiFi...");
    update_wifi_status(scheduler, "Trying saved WiFi...");
    if (try_saved_wifi(scheduler)) {
        return true;
    }
    
    // 步骤2: 尝试配置文件中的WiFi
    Serial.println("Step 2: Trying config WiFi...");
    update_wifi_status(scheduler, "Trying config WiFi...");
    if (try_config_wifis(scheduler)) {
        return true;
    }
    
    // 步骤3: 所有WiFi都失败，根据配置决定是否开启配置门户
    #if ENABLE_WIFI_MANAGER_PORTAL
        Serial.println("Step 3: All WiFi failed, starting config portal...");
        return start_config_portal(scheduler);
    #else
        Serial.println("All WiFi failed! WiFiManager portal is disabled.");
        update_wifi_status(scheduler, "WiFi connection failed");
        return false;
    #endif
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

