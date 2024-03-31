#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include <math.h>

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240
#define BUFFER_SIZE 2048 

// header that is same as python file
uint8_t header[4] = {0xAB, 0xCD, 0x12, 0x34};

void HSVtoRGB(float hue, float saturation, float value, uint16_t *red, uint16_t *green, uint16_t *blue);

void app_main() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUFFER_SIZE, 0, 0, NULL, 0);

    int hue_increment = 360 / SCREEN_HEIGHT;

    while(1) {
        // send the header
        uart_write_bytes(UART_NUM_0, (const char*)header, sizeof(header));
        
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            uint16_t color_data[SCREEN_WIDTH];
            float hue = (hue_increment * y) % 360;

            uint16_t red, green, blue;
            HSVtoRGB(hue, 100.0, 100.0, &red, &green, &blue);
            // combine into one number 5 bits of red, 6 bits of green, and 5 bits of blue
            uint16_t color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
            for (int x = 0; x < SCREEN_WIDTH; x++) {
                color_data[x] = color;
            }

            uart_write_bytes(UART_NUM_0, (const char*)&color_data, sizeof(color_data));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/**
 * Retreived then modified from https://github.com/Inseckto/HSV-to-RGB/blob/master/HSV2RGB.c
*/
void HSVtoRGB(float hue, float saturation, float value, uint16_t *red, uint16_t *green, uint16_t *blue) {
    float r = 0, g = 0, b = 0;

    float h = hue / 360;
    float s = saturation / 100;
    float v = value / 100;
    
    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);
    
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    
    *red = (uint16_t)(r * 255);
    *green = (uint16_t)(g * 255);
    *blue = (uint16_t)(b * 255);
}

