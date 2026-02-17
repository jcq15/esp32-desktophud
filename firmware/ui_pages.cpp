#include "ui_pages.h"
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

// 引用 display_hal.cpp 里那个全局 display 对象
extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;

void renderHome(const AppState& state) {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setRotation(1);

    int y = 30;
    int lineHeight = 25;
    
    // 标题 - 日期和星期
    display.setFont(&FreeMonoBold12pt7b);
    if (state.apiData.success && state.apiData.cal_date.length() > 0) {
        display.setCursor(20, y);
        display.print(state.apiData.cal_date);
        if (state.apiData.cal_weekday.length() > 0) {
            display.print(" ");
            display.print(state.apiData.cal_weekday);
        }
    } else {
        display.setCursor(20, y);
        display.print("Desktop HUD");
    }
    
    y += lineHeight + 5;
    
    // 农历和节日信息
    display.setFont(&FreeMono9pt7b);
    if (state.apiData.success) {
        if (state.apiData.cal_lunar.length() > 0) {
            display.setCursor(20, y);
            display.print(state.apiData.cal_lunar);
        }
        if (state.apiData.cal_extra.length() > 0) {
            display.print(" ");
            display.print(state.apiData.cal_extra);
        }
    }
    
    y += lineHeight + 10;
    
    // 天气信息
    display.setFont(&FreeMonoBold9pt7b);
    if (state.apiData.success && state.apiData.wx_city.length() > 0) {
        display.setCursor(20, y);
        display.print(state.apiData.wx_city);
        if (state.apiData.wx_t.length() > 0) {
            display.print(" ");
            display.print(state.apiData.wx_t);
            display.print("°C");
        }
        if (state.apiData.wx_text.length() > 0) {
            display.print(" ");
            display.print(state.apiData.wx_text);
        }
        
        y += lineHeight;
        if (state.apiData.wx_aqi.length() > 0) {
            display.setFont(&FreeMono9pt7b);
            display.setCursor(20, y);
            display.print("空气质量: ");
            display.print(state.apiData.wx_aqi);
        }
    }
    
    y += lineHeight + 10;
    
    // 笔记列表
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(20, y);
    display.print("待办事项:");
    
    y += lineHeight;
    display.setFont(&FreeMono9pt7b);
    if (state.apiData.success && state.apiData.note_count > 0) {
        for (int i = 0; i < state.apiData.note_count && y < 400; i++) {
            display.setCursor(30, y);
            display.print("- ");
            display.print(state.apiData.notes[i]);
            y += lineHeight - 5;
        }
    } else {
        display.setCursor(30, y);
        display.print("暂无待办事项");
    }
    
    // 状态信息（底部）
    y = 450;
    display.setFont(&FreeMono9pt7b);
    display.setCursor(20, y);
    if (state.wifiConnected) {
        display.print("WiFi: 已连接 | ");
    } else {
        display.print("WiFi: 未连接 | ");
    }
    display.print(state.statusMessage);
}
