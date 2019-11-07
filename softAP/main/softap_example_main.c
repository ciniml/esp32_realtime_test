/*  WiFi softAP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/udp.h"

/* The examples use WiFi configuration that you can set via 'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_MAX_STA_CONN       CONFIG_MAX_STA_CONN

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wifi softAP";

static volatile ip4_addr_t client_address;
static volatile bool is_client_connected = false;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR" join, AID=%d",
                 MAC2STR(event->event_info.sta_connected.mac),
                 event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:"MACSTR"leave, AID=%d",
                 MAC2STR(event->event_info.sta_disconnected.mac),
                 event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
        ESP_LOGI(TAG, "station:"IPSTR" assigned", IP2STR(&event->event_info.ap_staipassigned.ip));
        client_address = event->event_info.ap_staipassigned.ip;
        is_client_connected = true;
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_init_softap()
{
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
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
    
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap();

    // Setup GPIO to read button station
    gpio_config_t config_gpio_button;
    config_gpio_button.pin_bit_mask = (1ull<<37);
    config_gpio_button.mode = GPIO_MODE_INPUT;
    config_gpio_button.pull_up_en = GPIO_PULLUP_ENABLE;
    config_gpio_button.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config_gpio_button.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&config_gpio_button));


    const uint16_t buffer_size = 10240;
    udp_init();
    struct udp_pcb* pcb = udp_new();
    struct pbuf* buf = pbuf_alloc(PBUF_TRANSPORT, buffer_size, PBUF_RAM);
    for(uint16_t i = 0; i < buffer_size; i++) {
        pbuf_put_at(buf, i, i);
    }

    uint64_t start_time = esp_timer_get_time();
    size_t total_bytes_sent = 0;

    bool transfer_enabled = true;
    bool last_button_pressed = false;

    while(true) {
        bool is_button_pressed = gpio_get_level(GPIO_NUM_37) == 0;
        if( !last_button_pressed && is_button_pressed ) {
            transfer_enabled = !transfer_enabled;
            ESP_LOGI("MAIN", "transfer: %s", transfer_enabled ? "enabled" : "disabled");
        }
        last_button_pressed = is_button_pressed;

        if( is_client_connected ) {
            if( transfer_enabled ) {
                ip_addr_t address;
                address.type = IPADDR_TYPE_V4;
                address.u_addr.ip4 = client_address;

                err_t err = udp_sendto(pcb, buf, &address, 10000);
                
                if( err == 0 ) {
                    total_bytes_sent += buffer_size;
                }
            }
        
            uint64_t timestamp = esp_timer_get_time();
            uint64_t elapsed_us = timestamp - start_time;
            if( elapsed_us >= 1000000ul ) {
                ESP_LOGI("MAIN", "transfer rate: %0.2lf", (total_bytes_sent*1000000.0)/elapsed_us);
                start_time = timestamp;
                total_bytes_sent = 0;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
