#include "widget_calendar.h"
#include "widget_utils.h"

extern DataHub dataHub;

bool CalendarWidget::syncFromHub(const DataHub& hub) {
    return sync_version_from_hub(lastVersion, hub.calendar.version, isDirty);
}

void CalendarWidget::render(const Rect& area) {
    render_bitmap_widget(area, dataHub.calendar.bitmapBuffer);
}

