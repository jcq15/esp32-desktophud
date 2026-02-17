#include "data_hub.h"
#include <ArduinoJson.h>

bool DataHub::updateFromJson(const String& json) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        errorMessage = "JSON parse error: " + String(error.c_str());
        return false;  // 返回false表示JSON解析失败
    }
    
    // JSON解析成功，清除之前的错误信息
    errorMessage = "";
    
    bool updated = false;
    
    // 更新版本号
    if (doc.containsKey("wx_ver")) {
        uint32_t new_ver = doc["wx_ver"].as<uint32_t>();
        if (new_ver != versions.wx_ver) {
            versions.wx_ver = new_ver;
            updated = true;
        }
    }
    if (doc.containsKey("cal_ver")) {
        uint32_t new_ver = doc["cal_ver"].as<uint32_t>();
        if (new_ver != versions.cal_ver) {
            versions.cal_ver = new_ver;
            updated = true;
        }
    }
    if (doc.containsKey("note_ver")) {
        uint32_t new_ver = doc["note_ver"].as<uint32_t>();
        if (new_ver != versions.note_ver) {
            versions.note_ver = new_ver;
            updated = true;
        }
    }
    
    // 更新天气数据
    if (doc.containsKey("wx")) {
        JsonObject wx = doc["wx"];
        if (wx.containsKey("city")) weather.city = wx["city"].as<String>();
        if (wx.containsKey("t")) weather.temperature = String(wx["t"].as<int>());
        if (wx.containsKey("text")) weather.text = wx["text"].as<String>();
        if (wx.containsKey("aqi")) weather.aqi = wx["aqi"].as<String>();
        weather.version = versions.wx_ver;
    }
    
    // 更新日历数据
    if (doc.containsKey("cal")) {
        JsonObject cal = doc["cal"];
        if (cal.containsKey("date")) calendar.date = cal["date"].as<String>();
        if (cal.containsKey("weekday")) calendar.weekday = cal["weekday"].as<String>();
        if (cal.containsKey("lunar")) calendar.lunar = cal["lunar"].as<String>();
        if (cal.containsKey("extra")) calendar.extra = cal["extra"].as<String>();
        calendar.version = versions.cal_ver;
    }
    
    // 更新笔记数据
    if (doc.containsKey("note") && doc["note"].is<JsonArray>()) {
        JsonArray noteArray = doc["note"];
        notes.count = min(noteArray.size(), (size_t)10);
        for (int i = 0; i < notes.count; i++) {
            notes.notes[i] = noteArray[i].as<String>();
        }
        notes.version = versions.note_ver;
    }
    
    // 更新获取间隔（分钟）
    if (doc.containsKey("ttl")) {
        fetchIntervalMinutes = doc["ttl"].as<int>();  // TTL以分钟为单位
    }
    
    return updated;
}

