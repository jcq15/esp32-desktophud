#include "widget_weather.h"
#include "widget_utils.h"

extern DataHub dataHub;

bool WeatherWidget::syncFromHub(const DataHub& hub) {
    return sync_version_from_hub(lastVersion, hub.weather.version, isDirty);
}

void WeatherWidget::render(const Rect& area) {
    render_bitmap_widget(area, dataHub.weather.bitmapBuffer);
}

