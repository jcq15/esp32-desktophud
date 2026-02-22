#include "widget_free.h"
#include "widget_utils.h"
#include <GxEPD2_BW.h>
#include <epd/GxEPD2_750_T7.h>

extern GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display;
extern DataHub dataHub;

bool FreeWidget::syncFromHub(const DataHub& hub) {
    return sync_version_from_hub(lastVersion, hub.forecast.version, isDirty);
}

void FreeWidget::render(const Rect& area) {
    render_bitmap_widget(area, dataHub.forecast.bitmapBuffer);
}

