#include "widget_free.h"
#include "widget_utils.h"
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;

bool FreeWidget::syncFromHub(const DataHub& hub) {
    // 自由区域目前不需要同步数据
    return false;
}

void FreeWidget::render(const Rect& area) {
    // 清空区域
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    
    // 绘制边框（目前不画内容，只画边框）
    draw_widget_border(area);
}

