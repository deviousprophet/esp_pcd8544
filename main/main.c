#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pcd8544.h"

#define LCD_HOST       SPI2_HOST
#define PIN_NUM_MISO   -1  // Not used
#define PIN_NUM_MOSI   21
#define PIN_NUM_CLK    22

// To speed up transfers, every SPI transfer sends a bunch of lines. This define
// specifies how many. More means more memory use, but less overhead for setting
// up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

static void demo_gui(void* arg) {
    uint8_t inv = 0;

    pcd8544_io_config_t lcd_io_cfg = {
        .rst_gpio_num = 5,
        .ce_gpio_num  = 18,
        .dc_gpio_num  = 19,
        .bkl_gpio_num = 23,
    };

    pcd8544_init(LCD_HOST, &lcd_io_cfg);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        for (uint8_t i = 0; i <= 50; i += 1) {
            pcd8544_draw_circle(42, 25, i, true);
            pcd8544_flush();
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(25));
        }

        pcd8544_clear();
        pcd8544_invert(++inv % 2);
    }

    pcd8544_deinit();
    vTaskDelete(NULL);
}

void app_main(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num     = PIN_NUM_MISO,
        .mosi_io_num     = PIN_NUM_MOSI,
        .sclk_io_num     = PIN_NUM_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = PARALLEL_LINES * PCD8544_H_RES_MAX * 2 + 8,
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    xTaskCreate(demo_gui, "demo GUI", 4 * 1024, NULL, 10, NULL);
}
