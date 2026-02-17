#include "api_client.h"
#include "wifi_config.h"
#include "config/config.h"  // 从统一配置文件读取配置
#include <HTTPClient.h>
#include <ArduinoJson.h>

void ApiResponse::clear() {
    success = false;
    rawJson = "";
    cal_date = "";
    cal_weekday = "";
    cal_lunar = "";
    cal_extra = "";
    wx_city = "";
    wx_t = "";
    wx_text = "";
    wx_aqi = "";
    note_count = 0;
    for (int i = 0; i < 10; i++) {
        notes[i] = "";
    }
}

bool api_fetch_data(ApiResponse& response) {
    response.clear();
    
    if (!wifi_is_connected()) {
        Serial.println("WiFi未连接，无法获取API数据");
        return false;
    }

    HTTPClient http;
    http.begin(API_SERVER_URL);
    http.setTimeout(10000);  // 10秒超时
    
    // 如果配置了API Token，添加到请求头
    #ifdef API_TOKEN
    http.addHeader("Authorization", "Bearer " + String(API_TOKEN));
    #endif
    
    // 如果配置了API Key，添加到请求头
    #ifdef API_KEY
    http.addHeader("X-API-Key", String(API_KEY));
    #endif
    
    Serial.print("正在请求API: ");
    Serial.println(API_SERVER_URL);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        response.rawJson = payload;
        
        Serial.println("API响应:");
        Serial.println(payload);
        
        // 使用ArduinoJson解析，动态分配内存以适应JSON格式变化
        DynamicJsonDocument doc(4096);  // 根据实际JSON大小调整
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.print("JSON解析失败: ");
            Serial.println(error.c_str());
            http.end();
            return false;
        }
        
        // 灵活解析：检查字段是否存在再提取
        // 日历信息
        if (doc.containsKey("cal")) {
            JsonObject cal = doc["cal"];
            if (cal.containsKey("date")) response.cal_date = cal["date"].as<String>();
            if (cal.containsKey("weekday")) response.cal_weekday = cal["weekday"].as<String>();
            if (cal.containsKey("lunar")) response.cal_lunar = cal["lunar"].as<String>();
            if (cal.containsKey("extra")) response.cal_extra = cal["extra"].as<String>();
        }
        
        // 天气信息
        if (doc.containsKey("wx")) {
            JsonObject wx = doc["wx"];
            if (wx.containsKey("city")) response.wx_city = wx["city"].as<String>();
            if (wx.containsKey("t")) response.wx_t = String(wx["t"].as<int>());
            if (wx.containsKey("text")) response.wx_text = wx["text"].as<String>();
            if (wx.containsKey("aqi")) response.wx_aqi = wx["aqi"].as<String>();
        }
        
        // 笔记列表
        if (doc.containsKey("note") && doc["note"].is<JsonArray>()) {
            JsonArray noteArray = doc["note"];
            response.note_count = min(noteArray.size(), (size_t)10);  // 最多10条
            for (int i = 0; i < response.note_count; i++) {
                response.notes[i] = noteArray[i].as<String>();
            }
        }
        
        response.success = true;
        http.end();
        return true;
    } else {
        Serial.print("HTTP请求失败，错误代码: ");
        Serial.println(httpCode);
        http.end();
        return false;
    }
}

