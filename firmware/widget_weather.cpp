#include "widget_weather.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;
extern DataHub dataHub;

bool WeatherWidget::syncFromHub(const DataHub& hub) {
    if (hub.weather.version != lastVersion) {
        lastVersion = hub.weather.version;
        isDirty = true;
        return true;
    }
    return false;
}

void WeatherWidget::render(const Rect& area) {
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    int y = area.y + 30;
    
    // 城市和温度
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(area.x + 10, y);
    if (dataHub.weather.city.length() > 0) {
        display.print(dataHub.weather.city);
    }
    if (dataHub.weather.temperature.length() > 0) {
        display.print(" ");
        display.print(dataHub.weather.temperature);
        display.print("°C");
    }
    
    // 天气描述
    y += 30;
    display.setFont(&FreeMono9pt7b);
    display.setCursor(area.x + 10, y);
    if (dataHub.weather.text.length() > 0) {
        display.print(dataHub.weather.text);
    }
    
    // 空气质量
    y += 30;
    display.setCursor(area.x + 10, y);
    if (dataHub.weather.aqi.length() > 0) {
        display.print("AQI: ");
        display.print(dataHub.weather.aqi);
    }
}

