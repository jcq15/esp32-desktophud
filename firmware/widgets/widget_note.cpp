#include "widget_note.h"
#include "widget_utils.h"

extern DataHub dataHub;

bool NoteWidget::syncFromHub(const DataHub& hub) {
    return sync_version_from_hub(lastVersion, hub.notes.version, isDirty);
}

void NoteWidget::render(const Rect& area) {
    render_bitmap_widget(area, dataHub.notes.bitmapBuffer);
}

