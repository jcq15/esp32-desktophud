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
    
    // 设置很短的超时时间，只用于尝试已保存的WiFi（不开启配置门户）
    wm.setConfigPortalTimeout(10);  // 10秒超时
    wm.setSaveConnectTimeout(15);    // 连接超时15秒
    
    // 检查是否有已保存的WiFi配置
    // 使用WiFiManager的方法读取，但需要先初始化
    wm.setConfigPortalBlocking(false);  // 非阻塞模式
    
    // 尝试读取已保存的WiFi
    String savedSSID = wm.getWiFiSSID(true);  // true表示从NVS读取
    String savedPass = wm.getWiFiPass(true);
    
    // 验证读取到的数据是否有效
    if (savedSSID.length() == 0 || savedSSID.length() > 32) {
        Serial.println("No saved WiFi config found");
        return false;
    }
    
    // 检查SSID是否包含有效字符（避免乱码）
    bool isValid = true;
    for (int i = 0; i < savedSSID.length(); i++) {
        unsigned char c = savedSSID.charAt(i);
        // SSID应该是可打印ASCII字符（32-126）或UTF-8字符
        // 但为了简单，我们检查是否在合理范围内
        if (c < 32 && c != 0) {  // 允许null终止符，但不允许其他控制字符
            isValid = false;
            break;
        }
    }
    
    if (!isValid) {
        Serial.println("Invalid saved WiFi config (corrupted), clearing...");
        wm.resetSettings();  // 清除损坏的配置
        return false;
    }
    
    Serial.print("Trying saved WiFi: ");
    Serial.println(savedSSID);
    
    String statusMsg = "Connecting to " + savedSSID + "...";
    update_wifi_status(scheduler, statusMsg);
    
    // 使用WiFiManager的autoConnect来尝试连接已保存的WiFi
    // 注意：如果已保存WiFi失败，由于超时时间很短，不会开启配置门户
    WiFi.mode(WIFI_STA);
    
    // 使用空的AP名称，这样如果连接失败，不会开启配置门户
    // 但我们需要确保它真的尝试了已保存的WiFi
    bool connected = wm.autoConnect("", "");  // 空字符串表示不开启配置门户
    
    if (connected && WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected using saved WiFi! IP: ");
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
    // autoConnect会自动保存WiFi凭据到NVS
    bool connected = wm.autoConnect(WIFI_MANAGER_AP_NAME, WIFI_MANAGER_AP_PASSWORD);
    
    if (connected) {
        Serial.print("Connected via portal! IP: ");
        Serial.println(WiFi.localIP());
        
        // 验证WiFi凭据是否已保存
        String savedSSID = wm.getWiFiSSID(true);
        if (savedSSID.length() > 0 && savedSSID.length() <= 32) {
            Serial.print("WiFi credentials saved: ");
            Serial.println(savedSSID);
        } else {
            Serial.println("Warning: WiFi credentials may not be saved correctly");
        }
        
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

