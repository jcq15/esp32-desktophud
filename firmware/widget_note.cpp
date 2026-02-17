#include "widget_note.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;
extern DataHub dataHub;

bool NoteWidget::syncFromHub(const DataHub& hub) {
    if (hub.notes.version != lastVersion) {
        lastVersion = hub.notes.version;
        isDirty = true;
        return true;
    }
    return false;
}

void NoteWidget::render(const Rect& area) {
    display.fillRect(area.x, area.y, area.w, area.h, GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    int y = area.y + 30;
    
    // 标题
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(area.x + 10, y);
    display.print("待办事项:");
    
    // 笔记列表
    y += 30;
    display.setFont(&FreeMono9pt7b);
    if (dataHub.notes.count > 0) {
        for (int i = 0; i < dataHub.notes.count && y < area.y + area.h - 20; i++) {
            display.setCursor(area.x + 20, y);
            display.print("- ");
            display.print(dataHub.notes.notes[i]);
            y += 25;
        }
    } else {
        display.setCursor(area.x + 20, y);
        display.print("暂无待办事项");
    }
}

