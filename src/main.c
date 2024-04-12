#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/api.h"


#define WIDTH 240
#define HEIGHT 240
#define ROWS_PER_PACKET 120
#define PIXEL_SIZE 2 // 16 bits per pixel
#define FRAME_DELAY pdMS_TO_TICKS(200)
#define PACKET_DELAY pdMS_TO_TICKS(4)

#define EXAMPLE_ESP_WIFI_SSID "our-wifi"
#define EXAMPLE_ESP_WIFI_PASS "bearthecat"

static const char *TAG = "wifi station";

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "retry to connect to the AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s", ip4addr_ntoa((ip4_addr_t*)&event->ip_info.ip.addr));
    }
}

void wifi_init_sta(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}



static void tcp_server_task(void *pvParameters) {
    struct netconn *conn, *newconn;
    err_t err;
    uint16_t pixel_data[WIDTH * ROWS_PER_PACKET];  // Buffer for 4 rows of pixel data
    uint16_t color_offset = 0;

    conn = netconn_new(NETCONN_TCP);
    if (!conn) {
        ESP_LOGE(TAG, "Failed to create new connection");
        vTaskDelete(NULL);  // Terminate this task if connection cannot be created
    }

    err = netconn_bind(conn, NULL, 80);
    if (err != ERR_OK) {
        ESP_LOGE(TAG, "Binding failed: %s", lwip_strerr(err));
        netconn_delete(conn);
        vTaskDelete(NULL);  // Terminate this task if binding fails
    }
    netconn_listen(conn);

    while (true) {
        err = netconn_accept(conn, &newconn);
        if (err != ERR_OK) {
            ESP_LOGE(TAG, "Accept failed: %s", lwip_strerr(err));
            continue;  // Skip to next iteration to accept a new connection
        }

        // Connection established, send data until an error occurs
        while (true) {
            for (int y = 0; y < HEIGHT; y += ROWS_PER_PACKET) {
                for (int row = 0; row < ROWS_PER_PACKET; row++) {
                    for (int x = 0; x < WIDTH; x++) {
                        // Calculate color index based on the width of the bars
                        int color_index = ((x + color_offset) / 10) % 3;
                        uint16_t color = (color_index == 0) ? 0x0000 :
                                         (color_index == 1) ? 0xFFFF :
                                         0xF800; // Red
                        pixel_data[row * WIDTH + x] = color;
                    }
                }
                err = netconn_write(newconn, pixel_data, WIDTH * ROWS_PER_PACKET * PIXEL_SIZE, NETCONN_COPY);
                if (err != ERR_OK) {
                    ESP_LOGE(TAG, "Write failed: %s", lwip_strerr(err));
                    break;  // Break from the write loop on error
                }
                ESP_LOGI(TAG, "Sent row: %d", y);
                vTaskDelay(PACKET_DELAY);
            }
            if (err != ERR_OK) break;  // Exit the connection loop on write error
            color_offset = (color_offset + 1) % 30;  // Move the bars to the right, reset after 30 pixels
            vTaskDelay(FRAME_DELAY);
        }

        netconn_close(newconn);
        netconn_delete(newconn);
    }

    // Clean up connection objects if the loop exits (which in this case, it shouldn't)
    netconn_close(conn);
    netconn_delete(conn);
}



void app_main(void) {
    nvs_flash_init();
    wifi_init_sta();
    xTaskCreate(tcp_server_task, "tcp_server", 65520, NULL, 5, NULL);
}