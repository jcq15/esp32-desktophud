#include <Arduino.h>
#include "display_hal.h"
#include "ui_pages.h"
#include "app_state.h"

static AppState state;
static AppState* g_state = nullptr;

static void drawCb() {
    renderHome(*g_state);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    display_begin();

    g_state = &state;
    display_full_refresh(drawCb);
}

void loop() {
    delay(1000);
}
