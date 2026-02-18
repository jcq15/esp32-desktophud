#pragma once
#include <Arduino.h>
#include "../widgets/widget.h"
#include "data_hub.h"
#include "../widgets/widget_sentence.h"
#include "../widgets/widget_time.h"
#include "../widgets/widget_calendar.h"
#include "../widgets/widget_weather.h"
#include "../widgets/widget_note.h"
#include "../widgets/widget_status.h"
#include "../widgets/widget_free.h"

// 内容服务：负责从服务器获取数据
class ContentService {
public:
    static bool fetchIfNeeded(DataHub& hub, const String& apiUrl);
};

// 调度器：统一管理所有Widget的更新和渲染
class Scheduler {
private:
    DataHub dataHub;
    
    // 所有Widget
    SentenceWidget sentenceWidget;
    TimeWidget timeWidget;
    CalendarWidget calendarWidget;
    WeatherWidget weatherWidget;
    NoteWidget noteWidget;
    StatusWidget statusWidget;
    FreeWidget freeWidget;
    
    static const int WIDGET_COUNT = 7;
    Widget* widgets[WIDGET_COUNT];
    
    // 维护刷新计数
    unsigned long partialRefreshCount = 0;
    unsigned long lastMaintenanceRefresh = 0;
    static const unsigned long MAINTENANCE_INTERVAL = 3600000;  // 1小时
    static const unsigned long PARTIAL_REFRESH_BEFORE_MAINTENANCE = 1000;  // 每1000次局刷后全刷
    
    // 收集dirty区域
    void collectDirtyRegions();
    
    // 执行刷新
    void performRefresh();
    
public:
    // 临时变量：用于lambda回调（避免捕获变量）
    int currentRefreshWidgetIndex = -1;
    Rect currentRefreshRect;
    Scheduler();
    
    // 初始化
    void begin();
    
    // 首次启动时获取数据（在首次全屏刷新前调用）
    void performInitialFetch();
    
    // 主循环
    void loop();
    
    // 更新本地时间（从RTC）
    void updateLocalTime();
    
    // 更新WiFi状态
    void updateWifiStatus(bool connected);
    
    // 更新状态信息
    void updateStatusMessage(const String& message);
    
    // 获取widgets数组（供外部访问，用于首次全屏刷新）
    Widget** getWidgets() { return widgets; }
    
    // 获取widgets数量
    int getWidgetCount() const { return WIDGET_COUNT; }
    
    // 调试：手动触发全屏刷新
    void forceFullRefresh();
};

