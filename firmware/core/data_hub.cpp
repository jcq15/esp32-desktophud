#include "data_hub.h"
#include <ArduinoJson.h>

bool DataHub::updateFromJson(const String& json) {
    DynamicJsonDocument doc(512 * 1024); // 有8MB的PSRAM，这里开 512KB。返回的json会几十KB。
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        errorMessage = "JSON parse error: " + String(error.c_str());
        return false;  // 返回false表示JSON解析失败
    }
    
    // JSON解析成功，清除之前的错误信息
    errorMessage = "";
    
    bool updated = false;
    
    // 更新版本号
    if (doc.containsKey("sentence_ver")) {
        uint32_t new_ver = doc["sentence_ver"].as<uint32_t>();
        if (new_ver != versions.sentence_ver) {
            versions.sentence_ver = new_ver;
            updated = true;
        }
    }
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
    if (doc.containsKey("forecast_ver")) {
        uint32_t new_ver = doc["forecast_ver"].as<uint32_t>();
        if (new_ver != versions.forecast_ver) {
            versions.forecast_ver = new_ver;
            updated = true;
        }
    }
    
    // 更新句子数据（位图格式）
    if (doc.containsKey("sentence")) {
        JsonObject sentenceObj = doc["sentence"];
        if (sentenceObj.containsKey("buffer")) {
            // 检查format字段，如果存在且为bitmap，或者不存在format字段（兼容旧格式）
            bool isBitmap = !sentenceObj.containsKey("format") || sentenceObj["format"].as<String>() == "bitmap";
            if (isBitmap) {
                String newBuffer = sentenceObj["buffer"].as<String>();
                if (newBuffer != sentence.bitmapBuffer) {
                    sentence.bitmapBuffer = newBuffer;
                    Serial.print("[DataHub] Sentence bitmap updated, length: ");
                    Serial.println(newBuffer.length());
                    updated = true;
                }
            }
        }
        sentence.version = versions.sentence_ver;
    }
    
    // 更新天气数据（位图格式）
    if (doc.containsKey("wx")) {
        JsonObject wx = doc["wx"];
        if (wx.containsKey("buffer")) {
            // 检查format字段，如果存在且为bitmap，或者不存在format字段（兼容旧格式）
            bool isBitmap = !wx.containsKey("format") || wx["format"].as<String>() == "bitmap";
            if (isBitmap) {
                String newBuffer = wx["buffer"].as<String>();
                if (newBuffer != weather.bitmapBuffer) {
                    weather.bitmapBuffer = newBuffer;
                    Serial.print("[DataHub] Weather bitmap updated, length: ");
                    Serial.println(newBuffer.length());
                    updated = true;
                }
            }
        }
        weather.version = versions.wx_ver;
    }
    
    // 更新日历数据（位图格式）
    if (doc.containsKey("cal")) {
        JsonObject cal = doc["cal"];
        if (cal.containsKey("buffer")) {
            // 检查format字段，如果存在且为bitmap，或者不存在format字段（兼容旧格式）
            bool isBitmap = !cal.containsKey("format") || cal["format"].as<String>() == "bitmap";
            if (isBitmap) {
                String newBuffer = cal["buffer"].as<String>();
                if (newBuffer != calendar.bitmapBuffer) {
                    calendar.bitmapBuffer = newBuffer;
                    Serial.print("[DataHub] Calendar bitmap updated, length: ");
                    Serial.println(newBuffer.length());
                    updated = true;
                }
            }
        }
        calendar.version = versions.cal_ver;
    }
    
    // 更新笔记数据（位图格式）
    if (doc.containsKey("note")) {
        // 检查note是对象还是数组
        if (doc["note"].is<JsonObject>()) {
            JsonObject note = doc["note"];
            if (note.containsKey("buffer")) {
                // 检查format字段，如果存在且为bitmap，或者不存在format字段（兼容旧格式）
                bool isBitmap = !note.containsKey("format") || note["format"].as<String>() == "bitmap";
                if (isBitmap) {
                    String newBuffer = note["buffer"].as<String>();
                    if (newBuffer != notes.bitmapBuffer) {
                        notes.bitmapBuffer = newBuffer;
                        Serial.print("[DataHub] Note bitmap updated, length: ");
                        Serial.println(newBuffer.length());
                        updated = true;
                    }
                }
            }
        } else {
            // 如果是数组格式（旧格式），忽略
            Serial.println("[DataHub] Note is array format, skipping");
        }
        notes.version = versions.note_ver;
    }
    
    // 更新天气预报数据（位图格式）
    if (doc.containsKey("forecast")) {
        JsonObject forecastObj = doc["forecast"];
        if (forecastObj.containsKey("buffer")) {
            // 检查format字段，如果存在且为bitmap，或者不存在format字段（兼容旧格式）
            bool isBitmap = !forecastObj.containsKey("format") || forecastObj["format"].as<String>() == "bitmap";
            if (isBitmap) {
                String newBuffer = forecastObj["buffer"].as<String>();
                if (newBuffer != forecast.bitmapBuffer) {
                    forecast.bitmapBuffer = newBuffer;
                    Serial.print("[DataHub] Forecast bitmap updated, length: ");
                    Serial.println(newBuffer.length());
                    updated = true;
                }
            }
        }
        forecast.version = versions.forecast_ver;
    }
    
    // 更新获取间隔（分钟）
    if (doc.containsKey("ttl")) {
        fetchIntervalMinutes = doc["ttl"].as<int>();  // TTL以分钟为单位
    }
    
    return updated;
}

