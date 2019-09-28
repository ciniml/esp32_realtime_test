/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/timer.h"
#include "soc/timer_group_struct.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about one event 
 * - are we connected to the AP with an IP? */
const int WIFI_CONNECTED_BIT = BIT0;

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                esp_wifi_connect();
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                s_retry_num++;
                ESP_LOGI(TAG,"retry to connect to the AP");
            }
            ESP_LOGI(TAG,"connect to the AP fail\n");
            break;
        }
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
}

// This example demonstrates how a human readable table of run time stats
// information is generated from raw data provided by uxTaskGetSystemState().
// The human readable table is written to pcWriteBuffer
static void printRuntimeStats()
{
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    uint32_t ulTotalRunTime, ulStatsAsPercentage;

    // Take a snapshot of the number of tasks in case it changes while this
    // function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();

    // Allocate a TaskStatus_t structure for each task.  An array could be
    // allocated statically at compile time.
    pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

    if( pxTaskStatusArray != NULL )
    {
        ESP_LOGI("STAT", "Stat begins");
        // Generate raw status information about each task.
        uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );

        // For percentage calculations.
        ulTotalRunTime /= 100UL;

        // Avoid divide by zero errors.
        if( ulTotalRunTime > 0 )
        {
            // For each populated position in the pxTaskStatusArray array,
            // format the raw data as human readable ASCII data
            for( x = 0; x < uxArraySize; x++ )
            {
                // What percentage of the total run time has the task used?
                // This will always be rounded down to the nearest integer.
                // ulTotalRunTimeDiv100 has already been divided by 100.
                ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

                if( ulStatsAsPercentage > 0UL )
                {
                    ESP_LOGI("STAT", "%s\t\t%u\t\t%u%%", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage );
                }
                else
                {
                    // If the percentage is zero here then the task has
                    // consumed less than 1% of the total run time.
                    ESP_LOGI("STAT", "%s\t\t%u\t\t<1%%", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter );
                }
            }
        }

        // The array is no longer needed, free the memory it consumes.
        vPortFree( pxTaskStatusArray );
        ESP_LOGI("STAT", "Stat ends");
    }
}
//#define USE_HR_TIMER
#define USE_HARDWARE_TIMER

#ifdef USE_HR_TIMER
#define TIMER_COUNTER_PERIOD (1000)
#elif defined(USE_HARDWARE_TIMER)
#define TIMER_COUNTER_PERIOD ((uint64_t)((TIMER_BASE_CLK/1000)/40000))
#endif

static volatile int64_t last_timer_timestamp = 0;
#define NUM_INTERVALS 256
static volatile uint32_t intervals[NUM_INTERVALS] = {0};
static volatile uint32_t interval_index = 0;

#ifdef USE_HARDWARE_TIMER
static IRAM_ATTR void hardware_timer_isr(void* arg)
{
    uint32_t intr_status = TIMERG0.int_st_timers.val;
    TIMERG0.hw_timer[0].update = 1;
    if( (intr_status & 1) == 0 ) {
        return;
    }
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.int_clr_timers.t1 = 1;
    TIMERG0.hw_timer[0].alarm_high = (uint32_t) (TIMER_COUNTER_PERIOD >> 32);
    TIMERG0.hw_timer[0].alarm_low = (uint32_t) TIMER_COUNTER_PERIOD;
    
    TaskHandle_t timer_task = (TaskHandle_t)arg;
    xTaskNotifyFromISR(timer_task, 1, eSetBits, NULL);

    // Re-enable timer alarm
    TIMERG0.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
}

static IRAM_ATTR void timer_callback(void* arg);
static IRAM_ATTR void timer_task(void* arg)
{
    while(true) {
        uint32_t notification_value = 0;
        xTaskNotifyWait(0, 1, &notification_value, portMAX_DELAY);
        timer_callback(arg);
    }
}
#endif

static IRAM_ATTR void timer_callback(void* arg)
{    
    TaskHandle_t main_task = (TaskHandle_t)arg;
    int64_t timestamp = esp_timer_get_time();
    int64_t period = timestamp - last_timer_timestamp;
    if( last_timer_timestamp > 0 ) {
        if( interval_index < NUM_INTERVALS ) {
            if( period < 0xffffffffll ) {
                intervals[interval_index] = (uint32_t)period;
                interval_index++;
            }
        }
        else {
            xTaskNotify(main_task, 1, eSetBits);
        }
    }
    last_timer_timestamp = timestamp;
}

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

#ifdef USE_HR_TIMER
    esp_timer_handle_t handle = NULL;
    esp_timer_create_args_t create_args = {
        .callback = timer_callback,
        .arg = xTaskGetCurrentTaskHandle(),
        .dispatch_method = 0, //ESP_TIMER_TASK,
        .name = "TIMERSTAT",
    };
    ESP_ERROR_CHECK(esp_timer_create(&create_args, &handle));
    ESP_ERROR_CHECK(esp_timer_start_periodic(handle, 1000));
#elif defined(USE_HARDWARE_TIMER)
    TaskHandle_t timer_task_handle = NULL;
    xTaskCreatePinnedToCore(timer_task, "HW_TIMER", 4096, xTaskGetCurrentTaskHandle(), 9, &timer_task_handle, APP_CPU_NUM);
    timer_config_t timer_config = {
        .alarm_en = true,
        .counter_en = false,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = true,
        .divider = 40000,
    };
    ESP_ERROR_CHECK(timer_init(TIMER_GROUP_0, 0, &timer_config));
    ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP_0, 0, 0));
    ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP_0, 0, TIMER_COUNTER_PERIOD));
    ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP_0, 0, hardware_timer_isr, timer_task_handle, ESP_INTR_FLAG_IRAM, NULL));
    ESP_ERROR_CHECK(timer_start(TIMER_GROUP_0, 0));
#endif
    while(true) {
        uint32_t notification_value = 0;
        xTaskNotifyWait(0, 1, &notification_value, portMAX_DELAY);
        if( interval_index < NUM_INTERVALS ) {
            continue;
        }

        float sum_squared = 0;
        float sum = 0;
        uint32_t min = 0xffffffffu;
        uint32_t max = 0;
        for(uint32_t i = 0; i < NUM_INTERVALS; i++) {
            uint32_t x = intervals[i];
            if( x < min ) {
                min = x;
            }
            if( max < x ) {
                max = x;
            }
            float v = x;
            sum += v;
            sum_squared += v*v;
        }
        interval_index = 0;
        float average = sum/NUM_INTERVALS;
        float variance = sum_squared/NUM_INTERVALS - average*average;
        ESP_LOGI("TIMER", "min = %u, max = %u, average: %f, variance: %f", min, max, average, variance);
    }
}
