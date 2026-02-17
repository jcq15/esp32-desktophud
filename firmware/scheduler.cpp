#include "scheduler.h"
#include "wifi_config.h"
#include "config/config.h"
#include "display_hal.h"
#include <HTTPClient.h>
#include <time.h>

// 全局DataHub实例（供Widget使用）
DataHub dataHub;

// 全局Scheduler实例（供回调使用）
Scheduler* g_scheduler = nullptr;

bool ContentService::fetchIfNeeded(DataHub& hub, const String& apiUrl) {
    Serial.print("[ContentService] Fetching from: ");
    Serial.println(apiUrl);
    
    if (!wifi_is_connected()) {
        hub.errorMessage = "WiFi not connected";
        Serial.println("[ContentService] ERROR: WiFi not connected");
        return false;
    }
    
    HTTPClient http;
    http.begin(apiUrl);
    http.setTimeout(30000);
    
    #ifdef API_TOKEN
    http.addHeader("Authorization", "Bearer " + String(API_TOKEN));
    Serial.println("[ContentService] Using API_TOKEN");
    #endif
    
    #ifdef API_KEY
    http.addHeader("X-API-Key", String(API_KEY));
    Serial.println("[ContentService] Using API_KEY");
    #endif
    
    Serial.print("[ContentService] Sending GET request...");
    int httpCode = http.GET();
    Serial.print(" HTTP code: ");
    Serial.println(httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.print("[ContentService] Response length: ");
        Serial.println(payload.length());
        
        if (payload.length() == 0) {
            Serial.println("[ContentService] ERROR: Empty response");
            hub.errorMessage = "Empty response";
            http.end();
            return false;
        }
        
        // 尝试解析JSON
        // updateFromJson返回false可能有两种情况：
        // 1. JSON解析失败（errorMessage不为空）- 这是真正的错误
        // 2. JSON解析成功但数据没更新（errorMessage为空）- 这是正常的，不算错误
        bool result = hub.updateFromJson(payload);
        
        // 检查是否是JSON解析失败（真正的错误）
        if (!result && hub.errorMessage.length() > 0) {
            // JSON解析失败，这是真正的错误
            Serial.print("[ContentService] ERROR: JSON parse failed - ");
            Serial.println(hub.errorMessage);
            http.end();
            return false;
        }
        
        // JSON解析成功（无论数据是否更新都算成功）
        if (result) {
            Serial.println("[ContentService] JSON parsed successfully, data updated");
        } else {
            Serial.println("[ContentService] JSON parsed successfully, data unchanged (version same)");
        }
        
        // 更新同步时间（无论数据是否更新）
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
            hub.lastSyncTime = String(timeStr);
            Serial.print("[ContentService] Last sync time: ");
            Serial.println(hub.lastSyncTime);
        }
        
        // 清除错误信息（请求成功）
        hub.errorMessage = "";
        
        http.end();
        return true;  // HTTP成功 + JSON解析成功 = 请求成功
    } else {
        String errorMsg = "HTTP error: " + String(httpCode);
        hub.errorMessage = errorMsg;
        Serial.print("[ContentService] ERROR: ");
        Serial.println(errorMsg);
        
        // 输出更详细的错误信息
        if (httpCode < 0) {
            Serial.print("[ContentService] Error code meaning: ");
            switch (httpCode) {
                case HTTPC_ERROR_CONNECTION_REFUSED:
                    Serial.println("Connection refused");
                    break;
                case HTTPC_ERROR_SEND_HEADER_FAILED:
                    Serial.println("Send header failed");
                    break;
                case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
                    Serial.println("Send payload failed");
                    break;
                case HTTPC_ERROR_NOT_CONNECTED:
                    Serial.println("Not connected");
                    break;
                case HTTPC_ERROR_CONNECTION_LOST:
                    Serial.println("Connection lost");
                    break;
                case HTTPC_ERROR_NO_STREAM:
                    Serial.println("No stream");
                    break;
                case HTTPC_ERROR_NO_HTTP_SERVER:
                    Serial.println("No HTTP server");
                    break;
                case HTTPC_ERROR_TOO_LESS_RAM:
                    Serial.println("Too less RAM");
                    break;
                case HTTPC_ERROR_ENCODING:
                    Serial.println("Encoding error");
                    break;
                case HTTPC_ERROR_STREAM_WRITE:
                    Serial.println("Stream write error");
                    break;
                case HTTPC_ERROR_READ_TIMEOUT:
                    Serial.println("Read timeout");
                    break;
                default:
                    Serial.println("Unknown error");
                    break;
            }
        }
        
        http.end();
        return false;
    }
}

Scheduler::Scheduler() {
    widgets[0] = &timeWidget;
    widgets[1] = &calendarWidget;
    widgets[2] = &weatherWidget;
    widgets[3] = &noteWidget;
    widgets[4] = &statusWidget;
    g_scheduler = this;
}

void Scheduler::begin() {
    // 初始化DataHub（nextFetchTime会在首次调用shouldFetch()时自动初始化）
    g_scheduler = this;
}

void Scheduler::performInitialFetch() {
    // 首次启动时从服务器获取数据
    if (wifi_is_connected()) {
        bool updated = ContentService::fetchIfNeeded(dataHub, API_SERVER_URL);
        if (updated) {
            dataHub.markFetched();
            
            // 更新全局dataHub（供Widget使用）
            ::dataHub = dataHub;
            
            // 强制同步所有Widget的数据（首次启动）
            timeWidget.syncFromHub(dataHub);
            calendarWidget.syncFromHub(dataHub);
            weatherWidget.syncFromHub(dataHub);
            noteWidget.syncFromHub(dataHub);
            statusWidget.syncFromHub(dataHub);
        }
    }
}

void Scheduler::updateLocalTime() {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        dataHub.localTimeString = String(timeStr);
        // 同步到全局dataHub
        ::dataHub.localTimeString = dataHub.localTimeString;
    }
}

void Scheduler::updateWifiStatus(bool connected) {
    dataHub.wifiConnected = connected;
    // 同步到全局dataHub
    ::dataHub.wifiConnected = connected;
}

void Scheduler::updateStatusMessage(const String& message) {
    dataHub.statusMessage = message;
    // 同步到全局dataHub
    ::dataHub.statusMessage = message;
}

void Scheduler::collectDirtyRegions() {
    // 1. 本地tick：时间Widget从RTC更新（每分钟）
    if (timeWidget.shouldUpdate()) {
        updateLocalTime();
        timeWidget.isDirty = timeWidget.syncFromHub(dataHub);
        Serial.println("[Scheduler] TimeWidget is dirty? " + String(timeWidget.isDirty));
    }
    
    // 2. 远程fetch：统一获取服务器数据（每5分钟）
    if (dataHub.shouldFetch()) {
        unsigned long currentMillis = millis();
        
        // 检查是否需要重试（如果上次失败且还没到重试延迟时间）
        if (dataHub.fetchRetryCount > 0) {
            unsigned long timeSinceLastAttempt = currentMillis - dataHub.lastFetchAttemptTime;
            if (timeSinceLastAttempt < DataHub::FETCH_RETRY_DELAY_MS) {
                // 还没到重试时间，跳过这次循环
                Serial.print("[Scheduler] Waiting for retry (");
                Serial.print((DataHub::FETCH_RETRY_DELAY_MS - timeSinceLastAttempt) / 1000);
                Serial.println("s remaining)");
                return;
            }
            Serial.print("[Scheduler] Retrying fetch (attempt ");
            Serial.print(dataHub.fetchRetryCount + 1);
            Serial.print("/");
            Serial.print(DataHub::MAX_FETCH_RETRIES);
            Serial.println(")");
        } else {
            Serial.println("[Scheduler] Starting fetch...");
        }
        
        dataHub.lastFetchAttemptTime = currentMillis;
        bool updated = ContentService::fetchIfNeeded(dataHub, API_SERVER_URL);
        
        if (updated) {
            // 成功获取，重置重试计数并设置下次获取时间
            Serial.println("[Scheduler] Fetch successful!");
            dataHub.fetchRetryCount = 0;
            dataHub.markFetched();
            
            // 更新全局dataHub（供Widget使用）
            ::dataHub = dataHub;
            
            Serial.println("[Scheduler] Scheduled next fetch");
        } else {
            // 获取失败，增加重试计数
            dataHub.fetchRetryCount++;
            Serial.print("[Scheduler] Fetch failed (retry ");
            Serial.print(dataHub.fetchRetryCount);
            Serial.print("/");
            Serial.print(DataHub::MAX_FETCH_RETRIES);
            Serial.println(")");
            
            if (dataHub.fetchRetryCount >= DataHub::MAX_FETCH_RETRIES) {
                // 超过最大重试次数，跳过这次，直接设置下次请求时间
                Serial.println("[Scheduler] Max retries reached, skipping this fetch, scheduling next");
                dataHub.fetchRetryCount = 0;
                dataHub.markFetched();
                Serial.println("[Scheduler] Next fetch scheduled");
            } else {
                // 还有重试机会，等待重试延迟后再次尝试
                Serial.print("[Scheduler] Will retry in ");
                Serial.print(DataHub::FETCH_RETRY_DELAY_MS / 1000);
                Serial.println(" seconds");
            }
        }
    }
    
    // 3. 检查各个Widget是否需要更新
    // NoteWidget（每5分钟）
    if (noteWidget.shouldUpdate()) {
        if (noteWidget.syncFromHub(dataHub)) {
            noteWidget.isDirty = true;
        }
    }
    
    // WeatherWidget（每30分钟）
    if (weatherWidget.shouldUpdate()) {
        if (weatherWidget.syncFromHub(dataHub)) {
            weatherWidget.isDirty = true;
        }
    }
    
    // CalendarWidget（每天）
    if (calendarWidget.shouldUpdate()) {
        if (calendarWidget.syncFromHub(dataHub)) {
            calendarWidget.isDirty = true;
        }
    }
    
    // StatusWidget（每分钟）
    if (statusWidget.shouldUpdate()) {
        if (statusWidget.syncFromHub(dataHub)) {
            statusWidget.isDirty = true;
        }
    }
}

void Scheduler::performRefresh() {
    // 检查是否需要维护刷新（全屏刷新去残影）
    bool needMaintenance = false;
    unsigned long now = millis();
    
    if ((now - lastMaintenanceRefresh) >= MAINTENANCE_INTERVAL) {
        needMaintenance = true;
        lastMaintenanceRefresh = now;
    } else if (partialRefreshCount >= PARTIAL_REFRESH_BEFORE_MAINTENANCE) {
        needMaintenance = true;
        partialRefreshCount = 0;
    }
    
    if (needMaintenance) {
        // 全屏刷新
        display_full_refresh([]() {
            if (g_scheduler) {
                // 渲染所有Widget
                Widget** widgets = g_scheduler->getWidgets();
                int count = g_scheduler->getWidgetCount();
                for (int i = 0; i < count; i++) {
                    widgets[i]->render(widgets[i]->rect);
                }
            }
        });
        return;
    }
    
    // 局部刷新dirty区域
    for (int i = 0; i < WIDGET_COUNT; i++) {
        if (widgets[i]->isDirty) {
            widgets[i]->markUpdated(); // 标记为已更新，并安排下次更新时间（对齐到整点）
            if (widgets[i]->renderMode == RenderMode::PARTIAL) {
                // 将索引和矩形存储到成员变量中，供无捕获lambda使用
                currentRefreshWidgetIndex = i;
                currentRefreshRect = widgets[i]->rect;
                
                display_partial_refresh_area(
                    []() {
                        if (g_scheduler && g_scheduler->currentRefreshWidgetIndex >= 0) {
                            Widget** ws = g_scheduler->getWidgets();
                            ws[g_scheduler->currentRefreshWidgetIndex]->render(g_scheduler->currentRefreshRect);
                        }
                    },
                    currentRefreshRect.x,
                    currentRefreshRect.y,
                    currentRefreshRect.w,
                    currentRefreshRect.h
                );
                partialRefreshCount++;
            }
            widgets[i]->markUpdated();  // 再次强调，反正多调用一次没坏处
        }
    }
}

void Scheduler::loop() {
    unsigned long now = millis();
    
    // A. 先更新数据
    collectDirtyRegions();
    
    // B. 收集dirtyRegions（已在collectDirtyRegions中完成）
    
    // C. 决定怎么刷新并执行
    performRefresh();
    
    // D. 刷完清空dirty（已在performRefresh中完成）
    
    delay(100);
}

