#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pcd8544.h"
#include "pcd8544_fonts.h"
#include "sys/param.h"

#define LCD_HOST       SPI2_HOST
#define PIN_NUM_MOSI   21
#define PIN_NUM_CLK    22

// To speed up transfers, every SPI transfer sends a bunch of lines. This define
// specifies how many. More means more memory use, but less overhead for setting
// up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

static void draw_button(pcd8544_obj_align_t align, const char* context,
                        uint8_t x_gap, uint8_t y_gap) {
    if (!context) return;

    uint8_t b_height, b_width, b_start_x, b_start_y, b_end_x, b_end_y, t_x, t_y;

    if (x_gap < 1) x_gap = 1;
    if (y_gap < 1) y_gap = 1;

    b_width  = PCD8544_CHAR5x7_WIDTH * strlen(context) + x_gap * 2;
    b_height = PCD8544_CHAR5x7_HEIGHT + y_gap * 2;

    switch (align) {
        case PCD8544_ALIGN_LEFT:
            b_start_x = 0;
            b_start_y = (PCD8544_V_RES_MAX - 1 - b_height) / 2;
            break;

        case PCD8544_ALIGN_RIGHT:
            b_start_x = PCD8544_H_RES_MAX - 1 - b_width;
            b_start_y = (PCD8544_V_RES_MAX - 1 - b_height) / 2;
            break;

        case PCD8544_ALIGN_TOP:
            b_start_x = (PCD8544_H_RES_MAX - 1 - b_width) / 2;
            b_start_y = 0;
            break;

        case PCD8544_ALIGN_BOTTOM:
            b_start_x = (PCD8544_H_RES_MAX - 1 - b_width) / 2;
            b_start_y = PCD8544_V_RES_MAX - 1 - b_height;
            break;

        case PCD8544_ALIGN_TOP_LEFT:
            b_start_x = 0;
            b_start_y = 0;
            break;

        case PCD8544_ALIGN_TOP_RIGHT:
            b_start_x = PCD8544_H_RES_MAX - 1 - b_width;
            b_start_y = 0;
            break;

        case PCD8544_ALIGN_BOTTOM_LEFT:
            b_start_x = 0;
            b_start_y = PCD8544_V_RES_MAX - 1 - b_height;
            break;

        case PCD8544_ALIGN_BOTTOM_RIGHT:
            b_start_x = PCD8544_H_RES_MAX - 1 - b_width;
            b_start_y = PCD8544_V_RES_MAX - 1 - b_height;
            break;

        case PCD8544_ALIGN_CENTER:
        default:
            b_start_x = (PCD8544_H_RES_MAX - 1 - b_width) / 2;
            b_start_y = (PCD8544_V_RES_MAX - 1 - b_height) / 2;
            break;
    }

    b_end_x = b_start_x + b_width;
    b_end_y = b_start_y + b_height;

    t_x = b_start_x + x_gap + 1;
    t_y = b_start_y + y_gap + 1;

    pcd8544_draw_rectagle(b_start_x, b_start_y, b_end_x, b_end_y,
                          PCD8544_PIXEL_BLACK, false);

    pcd8544_goto_xy(t_x, t_y);
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, context);
    pcd8544_flush();
}

static void demo_gui(void* arg) {
    pcd8544_io_config_t lcd_io_cfg = {
        .rst_gpio_num = 5,
        .ce_gpio_num  = 18,
        .dc_gpio_num  = 19,
        .bkl_gpio_num = 23,
    };

    pcd8544_init(LCD_HOST, &lcd_io_cfg);
    pcd8544_set_backlight_fade(100, 2000, true);

    pcd8544_draw_rectagle(0, 0, 5, 5, PCD8544_PIXEL_BLACK, true);
    pcd8544_flush();

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        for (uint8_t i = 0; i < PCD8544_H_RES_MAX - 6; i += 2) {
            pcd8544_scroll(2, 0);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
        }

        for (uint8_t i = 0; i < PCD8544_V_RES_MAX - 6; i++) {
            pcd8544_scroll(0, 1);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
        }

        for (uint8_t i = 0; i < PCD8544_H_RES_MAX - 6; i += 2) {
            pcd8544_scroll(-2, 0);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
        }

        for (uint8_t i = 0; i < PCD8544_V_RES_MAX - 6; i++) {
            pcd8544_scroll(0, -1);
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
        }
    }

    pcd8544_deinit();
    vTaskDelete(NULL);
}

void app_main(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num     = -1,  // Not used
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
