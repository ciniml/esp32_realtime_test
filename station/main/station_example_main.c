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
#include "lwip/udp.h"

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
                    ESP_LOGI("STAT", "%s\t\t%u\t\t%u%%\t%u", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage, pxTaskStatusArray[x].uxCurrentPriority );
                }
                else
                {
                    // If the percentage is zero here then the task has
                    // consumed less than 1% of the total run time.
                    ESP_LOGI("STAT", "%s\t\t%u\t\t<1%%\t%u", pxTaskStatusArray[ x ].pcTaskName, pxTaskStatusArray[ x ].ulRunTimeCounter, pxTaskStatusArray[x].uxCurrentPriority );
                }
            }
        }

        // The array is no longer needed, free the memory it consumes.
        vPortFree( pxTaskStatusArray );
        ESP_LOGI("STAT", "Stat ends");
    }
}

#if CONFIG_TARGET_TIMER_TASK_DELAY
#define USE_TASK_DELAY
#elif CONFIG_TARGET_TIMER_HARDWARE
#define USE_HARDWARE_TIMER
#elif CONFIG_TARGET_TIMER_HIGH_RES
#define USE_HR_TIMER
#endif
#if CONFIG_TARGET_HARDWARE_TIMER_GROUP_0
#define TIMER_GROUP TIMER_GROUP_0
#define TIMERG TIMERG0
#elif CONFIG_TARGET_HARDWARE_TIMER_GROUP_1
#define TIMER_GROUP TIMER_GROUP_1
#define TIMERG TIMERG1
#endif

#if CONFIG_ENABLE_WIFI
#define ENABLE_WIFI
#endif

#if CONFIG_PLACE_CALLBACK_ON_IRAM
#define PLACE_CALLBACK_ON_IRAM
#endif


#if defined(USE_HR_TIMER) || defined(USE_TASK_DELAY)
#define TIMER_COUNTER_PERIOD (500)
#elif defined(USE_HARDWARE_TIMER)
#define TIMER_CLOCK_DIVIDER 400
#define TIMER_COUNTER_PERIOD ((uint64_t)100)
#endif

static volatile int64_t last_timer_timestamp = 0;
static volatile int64_t isr_timestamp = 0;
#define NUM_INTERVALS 16384
typedef struct {
    uint32_t interval;
    uint32_t delay;
} IntervalItem;
static volatile IntervalItem intervals[NUM_INTERVALS] = {0};
static volatile uint32_t interval_index = 0;

#ifdef PLACE_CALLBACK_ON_IRAM
#define CALLBACK_PLACE_ATTR IRAM_ATTR
#else
#define CALLBACK_PLACE_ATTR
#endif

#ifdef USE_HARDWARE_TIMER
static IRAM_ATTR void hardware_timer_isr(void* arg)
{
    if( TIMERG.int_raw.t0 == 0 ) {
        return;
    }
    // Clear interrupt flag.
    TIMERG.int_clr_timers.t0 = 1;
    // Clear timer counter
    TIMERG.hw_timer[0].load_high = 0;
    TIMERG.hw_timer[0].load_low = 0;
    TIMERG.hw_timer[0].reload = 0;  // set counter to zero.

    int64_t timestamp = esp_timer_get_time();

    // Re-enable timer alarm
    TIMERG.hw_timer[0].alarm_high = (uint32_t) (TIMER_COUNTER_PERIOD >> 32);
    TIMERG.hw_timer[0].alarm_low = (uint32_t) TIMER_COUNTER_PERIOD;
    TIMERG.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
    // Set timestamp
    if( interval_index < NUM_INTERVALS && intervals[interval_index].interval == 0 ) {
        isr_timestamp = timestamp;
        intervals[interval_index].interval = timestamp - last_timer_timestamp;
    }
    last_timer_timestamp = timestamp;

    // Notify
    TaskHandle_t timer_task = (TaskHandle_t)arg;
    xTaskNotifyFromISR(timer_task, 1, eSetBits, NULL);
    portYIELD_FROM_ISR();   // In FreeRTOS, the task currently running is not yielded even if a notification is sent to another task. Thus we have to yield the current task explicitly by calling portYIELD_FROM_ISR().
}

static CALLBACK_PLACE_ATTR void timer_task(void* arg)
{
    timer_config_t timer_config = {
        .alarm_en = true,
        .counter_en = false,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = true,
        .divider = TIMER_CLOCK_DIVIDER,
    };
    ESP_ERROR_CHECK(timer_init(TIMER_GROUP, 0, &timer_config));
    ESP_ERROR_CHECK(timer_set_counter_value(TIMER_GROUP, 0, 0));
    ESP_ERROR_CHECK(timer_set_alarm_value(TIMER_GROUP, 0, TIMER_COUNTER_PERIOD));
    ESP_ERROR_CHECK(timer_isr_register(TIMER_GROUP, 0, hardware_timer_isr, xTaskGetCurrentTaskHandle(), ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3, NULL));
    ESP_ERROR_CHECK(timer_start(TIMER_GROUP, 0));

    while(true) {
        uint32_t notification_value = 0;
        xTaskNotifyWait(0, 1, &notification_value, portMAX_DELAY);
        TaskHandle_t main_task = (TaskHandle_t)arg;
        int64_t timestamp = esp_timer_get_time();
        
        if( interval_index < NUM_INTERVALS ) {
            if( intervals[interval_index].interval != 0 ) {
                intervals[interval_index].delay = (uint32_t)(timestamp - isr_timestamp);
                interval_index++;
            }
        }
        else {
            xTaskNotify(main_task, 1, eSetBits);
        }
    }
}
#elif USE_TASK_DELAY

static CALLBACK_PLACE_ATTR void timer_task(void* arg)
{
    while(true) {
        TaskHandle_t main_task = (TaskHandle_t)arg;
        int64_t timestamp = esp_timer_get_time();
        int64_t period = timestamp - last_timer_timestamp;
        if( last_timer_timestamp > 0 ) {
            if( interval_index < NUM_INTERVALS ) {
                if( period < 0xffffffffll ) {
                    intervals[interval_index].interval = (uint32_t)period;
                    interval_index++;
                }
            }
            else {
                xTaskNotify(main_task, 1, eSetBits);
            }
        }
        last_timer_timestamp = timestamp;
    }
}
#elif USE_HR_TIMER
static IRAM_ATTR void timer_callback(void* arg)
{    
    TaskHandle_t main_task = (TaskHandle_t)arg;
    int64_t timestamp = esp_timer_get_time();
    int64_t period = timestamp - last_timer_timestamp;
    if( last_timer_timestamp > 0 ) {
        if( interval_index < NUM_INTERVALS ) {
            if( period < 0xffffffffll ) {
                intervals[interval_index].interval = (uint32_t)period;
                interval_index++;
            }
        }
        else {
            xTaskNotify(main_task, 1, eSetBits);
        }
    }
    last_timer_timestamp = timestamp;
}
#endif

static struct udp_pcb* udp_context = NULL;
static void udp_recv_handler(void* arg, struct udp_pcb* pcb, struct pbuf* p, const ip_addr_t* addr, uint16_t port)
{
    pbuf_free(p);
}
static void initialize_udp()
{
    udp_context = udp_new();
    udp_bind(udp_context, IPADDR_ANY, 10000);
    udp_recv(udp_context, &udp_recv_handler, NULL);
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
#ifdef ENABLE_WIFI
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    initialize_udp();
    ESP_LOGI(TAG, "Waiting AP connection...");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, 0, 0, portMAX_DELAY);
#endif
#ifdef USE_HR_TIMER
    ESP_LOGI(TAG, "Use high resolution timer");
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
    ESP_LOGI(TAG, "Use hardware timer, priority=%d, cpu=%d", CONFIG_HARDWARE_TIMER_TASK_PRIORITY, CONFIG_HARDWARE_TIMER_TASK_CPU);
    TaskHandle_t timer_task_handle = NULL;
    xTaskCreatePinnedToCore(timer_task, "HW_TIMER", 4096, xTaskGetCurrentTaskHandle(), CONFIG_HARDWARE_TIMER_TASK_PRIORITY, &timer_task_handle, CONFIG_HARDWARE_TIMER_TASK_CPU);
#endif
    while(true) {
        uint32_t notification_value = 0;
        xTaskNotifyWait(0, 1, &notification_value, portMAX_DELAY);
        if( interval_index < NUM_INTERVALS ) {
            continue;
        }

#if CONFIG_DUMP_RAW_DATA
        esp_log_level_set("*", ESP_LOG_NONE);
        printf("DUMP BEGIN:\n");
        for(uint32_t i = 0; i < NUM_INTERVALS; i++) {
            printf("%d,%d,%d\n", i, intervals[i].delay, intervals[i].interval);
            if( (i & 0xfff) == 0 ) {
                vTaskDelay(1);
            }
        }
        printf("DUMP END:\n");
        esp_log_level_set("*", ESP_LOG_INFO);
#else 
        float interval_sum_squared = 0;
        float interval_sum = 0;
        uint32_t interval_min = 0xffffffffu;
        uint32_t interval_max = 0;
        float delay_sum_squared = 0;
        float delay_sum = 0;
        uint32_t delay_min = 0xffffffffu;
        uint32_t delay_max = 0;
        for(uint32_t i = 0; i < NUM_INTERVALS; i++) {
            {
                uint32_t x = intervals[i].delay;
                float v = x;
                delay_min = x < delay_min ? x : delay_min;
                delay_max = x > delay_max ? x : delay_max;
                delay_sum += v;
                delay_sum_squared += v*v;
            }
            {
                uint32_t x = intervals[i].interval;
                float v = x;
                interval_min = x < interval_min ? x : interval_min;
                interval_max = x > interval_max ? x : interval_max;
                interval_sum += v;
                interval_sum_squared += v*v;
            }
        }
        float delay_average     = delay_sum/NUM_INTERVALS;
        float delay_variance    = delay_sum_squared/NUM_INTERVALS - delay_average*delay_average;
        float interval_average  = interval_sum/NUM_INTERVALS;
        float interval_variance = interval_sum_squared/NUM_INTERVALS - interval_average*interval_average;

        ESP_LOGI("TIMER", "delay:    min = %u, max = %u, average: %f, variance: %f", delay_min, delay_max, delay_average, delay_variance);
        ESP_LOGI("TIMER", "interval: min = %u, max = %u, average: %f, variance: %f", interval_min, interval_max, interval_average, interval_variance);
        printRuntimeStats();
#endif
        memset(intervals, 0, sizeof(intervals));
        interval_index = 0;
    }
}
