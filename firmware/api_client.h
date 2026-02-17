#pragma once
#include <Arduino.h>

// API响应数据结构（灵活存储JSON）
struct ApiResponse {
    bool success = false;
    String rawJson = "";  // 原始JSON字符串，用于调试
    
    // 使用动态键值对存储，适应JSON格式变化
    // 主要字段的快速访问
    String cal_date = "";
    String cal_weekday = "";
    String cal_lunar = "";
    String cal_extra = "";
    
    String wx_city = "";
    String wx_t = "";
    String wx_text = "";
    String wx_aqi = "";
    
    // 笔记列表
    String notes[10];  // 最多存储10条笔记
    int note_count = 0;
    
    // 清空数据
    void clear();
};

// 调用API并解析JSON
bool api_fetch_data(ApiResponse& response);

