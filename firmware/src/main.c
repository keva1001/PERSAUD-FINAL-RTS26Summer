/*
 * Finaol Project— Dual-Core IPC Pipeline Architecture
 *
 * Theme: ICU Bedside ECG Telemetry & Nurse Call System
 *
 * Pipeline Architecture (Core 1 - Real-Time Tasks):
 *   producer_task    (Task A - 20 Hz / 50 ms):
 *                      Acquires medical telemetry samples (ECG, HR, SpO2), tracks WCET,
 *                      and enqueues items into data_q. Sets EV_BIT_DATA_PRODUCED.
 *   consumer_task    (Task B - 100 ms max wait):
 *                      Dequeues samples, runs arrhythmia analysis, tracks WCET, and
 *                      sets EV_BIT_DATA_PROCESSED.
 *   coordinator_task (Task C - Rendezvous):
 *                      Waits for both EV_BIT_DATA_PRODUCED & EV_BIT_DATA_PROCESSED.
 *                      Tracks WCET and triggers responder_task via direct notification.
 *   responder_task   (Task D - Alert Actuator):
 *                      Waits for task notifications (from Coordinator or Nurse Call ISR).
 *                      Tracks WCET and executes alert response.
 *
 * Observability Plane (Core 0):
 *   USE_WEBSERVER = 0 -> Serial Monitor Task (1 Hz terminal metrics).
 *   USE_WEBSERVER = 1 -> Embedded HTTP Web Server (HTML dashboard with auto-refresh).
 */

#ifndef USE_WEBSERVER
#define USE_WEBSERVER 0
#endif

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_attr.h"

#if USE_WEBSERVER
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASS ""
#define HTTP_PORT 80
#endif

#define BUTTON_GPIO GPIO_NUM_18

static const char *TAG = "ICU_IPC_SYS";

/* ---------- Medical Data Structures ---------- */
typedef enum {
    RHYTHM_NORMAL = 0,
    RHYTHM_ARRHYTHMIA
} rhythm_verdict_t;

typedef struct {
    uint32_t timestamp_ms;   //sample capture time 
    uint32_t patient_id;     //bed/Patient Identifier 
    uint16_t heart_rate_bpm; //vital Sign: Heart Rate (BPM) 
    uint16_t spo2_percent;   //vital Sign: Blood Oxygen (%) 
    int16_t  ecg_mv;         //simulated raw ECG amplitude (mV) 
} medical_telemetry_t;

/* Queue Configuration Defense:
 * Depth = 10, Item Size = sizeof(medical_telemetry_t).
 * At 20 Hz (50 ms rate), depth 10 provides a 500 ms safety buffer against jitter.
 */
#define QUEUE_DEPTH 10

//IPC handles 
static QueueHandle_t      data_q           = NULL;
static EventGroupHandle_t evt_group        = NULL;
static TaskHandle_t       responder_handle = NULL;


#define EV_BIT_DATA_PRODUCED  (1 << 0)
#define EV_BIT_DATA_PROCESSED (1 << 1)

//per task metrics and heartbeats 
static volatile uint32_t hb_a = 0; //producer -> task A
static volatile uint32_t hb_b = 0; //consumer -> task B
static volatile uint32_t hb_c = 0; //coordinator -> task C
static volatile uint32_t hb_d = 0; //responder -> task D

static volatile uint64_t wcet_a_max_us = 0;
static volatile uint64_t wcet_b_max_us = 0;
static volatile uint64_t wcet_c_max_us = 0;
static volatile uint64_t wcet_d_max_us = 0;

//global var for state tracking 
static volatile uint32_t dropped_samples   = 0;
static volatile uint32_t arrhythmia_alerts = 0;
static volatile uint32_t manual_nurse_calls= 0;

static volatile rhythm_verdict_t     last_verdict = RHYTHM_NORMAL;
static volatile medical_telemetry_t  last_sample  = {0};

//utility macro for WCET measurements 
#define MEASURE_WCET(start_time, max_var) do {                            \
    uint64_t elapsed = esp_timer_get_time() - start_time;                 \
    if (elapsed > max_var) { max_var = elapsed; }                         \
} while(0)

/* 
 *  PRODUCER TASK -> Task A (Core 1) — Telemetry Sampler
 */
static void producer_task(void *arg)
{
    uint32_t sample_id = 0;

    for (;;) {
        uint64_t t_start = esp_timer_get_time();

        medical_telemetry_t sample = {
            .timestamp_ms   = (uint32_t)(t_start / 1000),
            .patient_id     = 104, /* Bed 104 */
            .heart_rate_bpm = 60 + (sample_id % 55), //60 - 114 BPM
            .spo2_percent   = 99 - (sample_id % 6),  //94% - 99% SpO2 
            .ecg_mv         = (int16_t)((sample_id % 20) * 10 - 100)
        };

        if (xQueueSend(data_q, &sample, pdMS_TO_TICKS(10)) == pdTRUE) {
            xEventGroupSetBits(evt_group, EV_BIT_DATA_PRODUCED);
        } else {
            dropped_samples++;
            ESP_LOGW(TAG, "[TASK A] Telemetry queue full! Dropping sample.");
        }

        sample_id++;
        hb_a++;
        MEASURE_WCET(t_start, wcet_a_max_us);

        vTaskDelay(pdMS_TO_TICKS(50)); //20 Hz sampling
    }
}

/*
 *  CONSUMER TASK -> Task B (Core 1) — Arrhythmia Processor
*/
static void consumer_task(void *arg)
{
    medical_telemetry_t item;

    for (;;) {
        if (xQueueReceive(data_q, &item, pdMS_TO_TICKS(100)) == pdTRUE) {
            uint64_t t_start = esp_timer_get_time();

            last_sample = item;

            // arrhythmia and hypoxemia threshold logic
            if (item.heart_rate_bpm > 100 || item.spo2_percent < 95) {
                last_verdict = RHYTHM_ARRHYTHMIA;
                arrhythmia_alerts++;
            } else {
                last_verdict = RHYTHM_NORMAL;
            }

            xEventGroupSetBits(evt_group, EV_BIT_DATA_PROCESSED);
            hb_b++;
            MEASURE_WCET(t_start, wcet_b_max_us);
        }
    }
}

/*
 *  COORDINATOR TASK -> Task C (Core 1) — Pipeline Rendezvous
*/
static void coordinator_task(void *arg)
{
    const EventBits_t wait_mask = EV_BIT_DATA_PRODUCED | EV_BIT_DATA_PROCESSED;

    for (;;) {
        EventBits_t got = xEventGroupWaitBits(evt_group, wait_mask,
                                              pdTRUE, //clears on exit 
                                              pdTRUE, //waits for all 
                                              portMAX_DELAY);
        if ((got & wait_mask) == wait_mask) {
            uint64_t t_start = esp_timer_get_time();

            if (responder_handle != NULL) {
                xTaskNotifyGive(responder_handle);
            }
            hb_c++;

            MEASURE_WCET(t_start, wcet_c_max_us);;
        }
    }
}

/*
 *  RESPONDER TASK -> Task D (Core 1) — Alarm Dispatch Actuator
 */
static void responder_task(void *arg)
{
    for (;;) {
        uint32_t n = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (n == 0) continue;

        uint64_t t_start = esp_timer_get_time();

        if (last_verdict == RHYTHM_ARRHYTHMIA) {
            ESP_LOGW(TAG, "[ACTUATOR] CRITICAL VITAL ALARM! Bed: %lu | HR: %u BPM | SpO2: %u%% | Triggers: %lu",
                     (unsigned long)last_sample.patient_id,
                     last_sample.heart_rate_bpm,
                     last_sample.spo2_percent,
                     (unsigned long)n);
        } else {
            ESP_LOGI(TAG, "[ACTUATOR] Routine cycle check normal. Bed: %lu | HR: %u BPM | SpO2: %u%%",
                     (unsigned long)last_sample.patient_id,
                     last_sample.heart_rate_bpm,
                     last_sample.spo2_percent);
        }

        hb_d++;
        MEASURE_WCET(t_start, wcet_d_max_us);
    }
}

/*
 *  BUTTON ISR — Nurse Call/Direct Override Interrupt
*/
static volatile int64_t last_edge_us = 0;
static void IRAM_ATTR button_isr(void *arg)
{
    int64_t now = esp_timer_get_time();
    if (now - last_edge_us < 200000) return; 
    last_edge_us = now;

    manual_nurse_calls++;

    if (responder_handle != NULL) {
        BaseType_t woken = pdFALSE;
        vTaskNotifyGiveFromISR(responder_handle, &woken);
        portYIELD_FROM_ISR(woken);
    }
}

#if USE_WEBSERVER
/* ============================================================
 *  WEB MONITOR (Core 0) [USE_WEBSERVER = 1]
 * ============================================================ */
static esp_err_t handle_root(httpd_req_t *req)
{
    UBaseType_t depth = uxQueueMessagesWaiting(data_q);
    EventBits_t bits  = xEventGroupGetBits(evt_group);

    char buf[2048];
    int n = snprintf(buf, sizeof(buf),
        "<!DOCTYPE html>"
        "<html lang=\"en\"><head>"
        "<meta charset=\"utf-8\"><meta http-equiv=\"refresh\" content=\"1\">"
        "<title>ICU ECG Monitor · Dual-Core IPC Pipeline</title>"
        "<style>"
        "  body { font-family: -apple-system, sans-serif; background: #FAFAF5; "
        "         color: #1A1A1A; padding: 1.5rem; }"
        "  h1 { color: #6B4F09; border-bottom: 3px solid #FFC904; "
        "       display: inline-block; padding-bottom: 4px; }"
        "  table { border-collapse: collapse; margin: 1rem 0; width: 100%%; max-width: 800px; }"
        "  th { background: #1A1A1A; color: #FFC904; padding: 8px 14px; "
        "       text-align: left; font-size: 12px; text-transform: uppercase; }"
        "  td { padding: 8px 14px; border-bottom: 1px solid #ddd; }"
        "  td.num { font-variant-numeric: tabular-nums; font-weight: 700; color: #6B4F09; }"
        "  .card { background: white; padding: 15px; margin-bottom: 15px; border-radius: 6px; "
        "          box-shadow: 0 2px 5px rgba(0,0,0,0.08); max-width: 800px; }"
        "  .alert { color: #e74c3c; font-weight: bold; }"
        "  .normal { color: #2ecc71; font-weight: bold; }"
        "</style></head>"
        "<body>"
        "<h1>ICU ECG Telemetry &mdash; Dual-Core IPC Pipeline</h1>"
        "<div class=\"card\">"
        "  <h3>Current Telemetry Payload (Bed %lu)</h3>"
        "  <p>Status: <span class=\"%s\">%s</span></p>"
        "  <p><b>Heart Rate:</b> %u BPM | <b>SpO2:</b> %u%% | <b>ECG Wave:</b> %d mV</p>"
        "  <p><b>IPC Queue Depth:</b> %u / %d | <b>Event Bits:</b> 0x%02X</p>"
        "</div>"
        "<table>"
        "<thead><tr><th>Task</th><th>Role</th><th>Period / Trigger</th>"
        "<th>Priority</th><th>Heartbeats</th><th>WCET (&micro;s)</th></tr></thead>"
        "<tbody>"
        "<tr><td>Task A &mdash; Telemetry Sampler</td><td>Producer</td>"
            "<td>50 ms</td><td>8</td>"
            "<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
        "<tr><td>Task B &mdash; Arrhythmia Process</td><td>Consumer</td>"
            "<td>Queue Event</td><td>8</td>"
            "<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
        "<tr><td>Task C &mdash; Pipeline Coordinator</td><td>Rendezvous</td>"
            "<td>Event Group</td><td>9</td>"
            "<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
        "<tr><td>Task D &mdash; Alarm Dispatcher</td><td>Actuator</td>"
            "<td>Notification</td><td>12</td>"
            "<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
        "</tbody></table>"
        "<div class=\"card\">"
        "  <p><b>Auto Arrhythmia Alerts:</b> %lu | <b>Manual Nurse Calls:</b> %lu | <b>Dropped Samples:</b> %lu</p>"
        "</div>"
        "<p style=\"font-size: 0.85rem; color: #666;\">Auto-refreshing every 1 s &mdash; heartbeats must grow monotonically.</p>"
        "</body></html>",
        (unsigned long)last_sample.patient_id,
        (last_verdict == RHYTHM_ARRHYTHMIA) ? "alert" : "normal",
        (last_verdict == RHYTHM_ARRHYTHMIA) ? "ARRHYTHMIA DETECTED" : "RHYTHM NORMAL",
        last_sample.heart_rate_bpm, last_sample.spo2_percent, last_sample.ecg_mv,
        (unsigned)depth, QUEUE_DEPTH, (unsigned)bits,
        (unsigned long)hb_a, (unsigned long long)wcet_a_max_us,
        (unsigned long)hb_b, (unsigned long long)wcet_b_max_us,
        (unsigned long)hb_c, (unsigned long long)wcet_c_max_us,
        (unsigned long)hb_d, (unsigned long long)wcet_d_max_us,
        (unsigned long)arrhythmia_alerts, (unsigned long)manual_nurse_calls, (unsigned long)dropped_samples);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, buf, n);
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port   = HTTP_PORT;
    cfg.core_id       = 0;
    cfg.task_priority = 5;
    cfg.stack_size    = 8192;
    httpd_handle_t s   = NULL;
    if (httpd_start(&s, &cfg) == ESP_OK) {
        httpd_uri_t root = { .uri="/", .method=HTTP_GET,
                             .handler=handle_root, .user_ctx=NULL };
        httpd_register_uri_handler(s, &root);
        ESP_LOGI(TAG, "[WEBMON] HTTP server started on port %d", HTTP_PORT);
    }
    return s;
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if      (base == WIFI_EVENT && id == WIFI_EVENT_STA_START)        esp_wifi_connect();
    else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) esp_wifi_connect();
    else if (base == IP_EVENT   && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "[WEBMON] IP Address assigned: " IPSTR, IP2STR(&e->ip_info.ip));
        start_webserver();
    }
}

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,   wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
    wifi_config_t cfg = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS,
                                   .threshold.authmode = WIFI_AUTH_OPEN } };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void webmonitor_task(void *arg)
{
    wifi_init_sta();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

#else  /* USE_WEBSERVER == 0 */

/* ============================================================
 *  TERMINAL MONITOR (Core 0) [USE_WEBSERVER = 0]
 * ============================================================ */
static void task_monitor(void *arg)
{
    TickType_t last = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000);

    for (;;) {
        UBaseType_t depth = uxQueueMessagesWaiting(data_q);
        EventBits_t bits  = xEventGroupGetBits(evt_group);

        printf("\n=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline ===\n");
        printf("Telemetry: Status=%s | HR=%u BPM | SpO2=%u%% | ECG=%d mV\n",
               (last_verdict == RHYTHM_ARRHYTHMIA) ? "ARRHYTHMIA" : "NORMAL",
               last_sample.heart_rate_bpm, last_sample.spo2_percent, last_sample.ecg_mv);
        printf("IPC State: Queue Depth=%u/%d | EventBits=0x%02X\n", (unsigned)depth, QUEUE_DEPTH, (unsigned)bits);
        printf("%-30s %-8s %-9s %-12s %-10s\n",
               "Task", "Period", "Priority", "Heartbeats", "WCET(us)");
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
               "A: Telemetry Sampler (Prod)", "50 ms",   8, (unsigned long)hb_a, (unsigned long long)wcet_a_max_us);
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
               "B: Arrhythmia Process (Cons)", "Queue",   8, (unsigned long)hb_b, (unsigned long long)wcet_b_max_us);
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
               "C: Pipeline Coordinator",     "Rendezvous", 9, (unsigned long)hb_c, (unsigned long long)wcet_c_max_us);
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
               "D: Alarm Dispatch Actuator",  "Notif",   12, (unsigned long)hb_d, (unsigned long long)wcet_d_max_us);
        printf("Stats: Auto Alerts=%lu | Nurse Calls=%lu | Drops=%lu\n",
               (unsigned long)arrhythmia_alerts, (unsigned long)manual_nurse_calls, (unsigned long)dropped_samples);

        vTaskDelayUntil(&last, period);
    }
}
#endif /* USE_WEBSERVER */

// System Initialization & Execution Core Placement

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "[ICU Bedside Telemetry] starting — Dual-Core IPC Pipeline");

#if USE_WEBSERVER
    ESP_LOGI(TAG, "Monitor Mode: WEB (USE_WEBSERVER=1) — Core 0");
#else
    ESP_LOGI(TAG, "Monitor Mode: SERIAL (USE_WEBSERVER=0) — Core 0");
#endif

    /* 1. Allocate FreeRTOS primitives */
    data_q    = xQueueCreate(QUEUE_DEPTH, sizeof(medical_telemetry_t));
    evt_group = xEventGroupCreate();

    /* 2. Real-Time Workload Core Allocation (Core 1) */
    xTaskCreatePinnedToCore(producer_task,    "prod_task",  4096, NULL,  8, NULL,              APP_CPU_NUM);
    xTaskCreatePinnedToCore(consumer_task,    "cons_task",  4096, NULL,  8, NULL,              APP_CPU_NUM);
    xTaskCreatePinnedToCore(coordinator_task, "coord_task", 4096, NULL,  9, NULL,              APP_CPU_NUM);
    xTaskCreatePinnedToCore(responder_task,   "resp_task",  4096, NULL, 12, &responder_handle, APP_CPU_NUM);

    /* 3. System Observability Plane (Core 0) */
#if USE_WEBSERVER
    xTaskCreatePinnedToCore(webmonitor_task,  "webmon_task", 4096, NULL, 4, NULL, PRO_CPU_NUM);
#else
    xTaskCreatePinnedToCore(task_monitor,     "mon_task",    4096, NULL, 4, NULL, PRO_CPU_NUM);
#endif

    /* 4. Hardware Trigger ISR (Nurse Call Button) */
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);
}
