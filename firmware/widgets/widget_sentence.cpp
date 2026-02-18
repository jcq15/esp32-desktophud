#include "widget_sentence.h"
#include "widget_utils.h"

extern DataHub dataHub;

bool SentenceWidget::syncFromHub(const DataHub& hub) {
    return sync_version_from_hub(lastVersion, hub.sentence.version, isDirty);
}

void SentenceWidget::render(const Rect& area) {
    render_bitmap_widget(area, dataHub.sentence.bitmapBuffer);
}

