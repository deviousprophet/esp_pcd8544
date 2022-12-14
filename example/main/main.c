#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pcd8544.h"

static const char* TAG = "pcd8544_demo";

#define DEMO_TIME_MS   3000  // Each demo will take 3 seconds long to run

// To speed up transfers, every SPI transfer sends a bunch of lines. This define
// specifies how many. More means more memory use, but less overhead for setting
// up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 16

/* Espressif's Logo */
DRAM_ATTR static const uint8_t demo_logo[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0,
    0x60, 0x30, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF0, 0xF8, 0xF8, 0xF8,
    0xF8, 0xF0, 0xF0, 0xF0, 0xE0, 0xE0, 0xE3, 0xC3, 0xC6, 0x86, 0x8E, 0x0E,
    0x1E, 0x1E, 0x3E, 0x7C, 0xFC, 0xF8, 0xF8, 0xF0, 0xF0, 0xE0, 0xC0, 0x80,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xF0, 0x1C, 0x07, 0x01,
    0x80, 0xC0, 0xE0, 0xE0, 0xE0, 0xE1, 0xE1, 0xE1, 0xE1, 0xE1, 0xE3, 0xE3,
    0xC3, 0xC3, 0x87, 0x87, 0x8F, 0x0F, 0x1F, 0x1F, 0x3F, 0x7F, 0x7F, 0xFF,
    0xFE, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC1, 0x83, 0x07, 0x0F, 0x3F, 0x7F,
    0xFF, 0xFE, 0xF8, 0xE0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x30, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0xC7, 0x87, 0x87, 0x87, 0x0F, 0x0F,
    0x1F, 0x1F, 0x3F, 0x3F, 0x7F, 0xFF, 0xFE, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0,
    0xC1, 0x03, 0x07, 0x1F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFE, 0xFC, 0xF0, 0xE0,
    0x00, 0x03, 0x0F, 0x3F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xE0, 0x80, 0x00, 0x01,
    0x03, 0x07, 0x0F, 0x0F, 0x0F, 0x1F, 0x1F, 0x1F, 0x3F, 0x3F, 0x7F, 0xFF,
    0xFE, 0xFE, 0xFC, 0xF8, 0xF0, 0xC0, 0x01, 0x07, 0x0F, 0x3F, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFC, 0xF0, 0x80, 0x00, 0x03, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFC, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x18,
    0x60, 0xC0, 0x80, 0x08, 0x3E, 0x7E, 0x7E, 0x7E, 0x3C, 0x00, 0x00, 0x00,
    0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F,
    0x0F, 0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x01, 0x03, 0x02, 0x04, 0x0C, 0x08, 0x10, 0x10, 0x20, 0x20,
    0x23, 0x43, 0x47, 0x47, 0x47, 0x07, 0x03, 0x00, 0x80, 0x80, 0x02, 0x43,
    0x43, 0x47, 0x47, 0x43, 0x23, 0x20, 0x30, 0x10, 0x18, 0x08, 0x0C, 0x06,
    0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void draw_bitmap_demo(void) {
    ESP_LOGI(TAG, "Running draw bitmap demo");
    pcd8544_clear();
    pcd8544_draw_bitmap(demo_logo);
    pcd8544_flush();
    vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS));
}

void draw_string_demo(void) {
    ESP_LOGI(TAG, "Running string demo");
    pcd8544_clear();
    pcd8544_goto_xy(10, 20);
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, "Demo string");
    pcd8544_flush();
    vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS));
}

void draw_pixel_demo(void) {
    ESP_LOGI(TAG, "Running draw pixel demo");
    pcd8544_clear();
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, "Demo pixel");
    pcd8544_draw_pixel(42, 25, PCD8544_PIXEL_BLACK);
    pcd8544_flush();
    vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS));
}

void draw_line_demo(void) {
    ESP_LOGI(TAG, "Running draw line demo");
    pcd8544_clear();
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, "Demo line");
    pcd8544_draw_line(22, 15, 62, 35, PCD8544_PIXEL_BLACK);
    pcd8544_flush();
    vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS));
}

void draw_rectangle_demo(void) {
    ESP_LOGI(TAG, "Running draw rectangle demo");
    pcd8544_clear();
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, "Demo rectangle");
    pcd8544_draw_rectagle(11, 23, 31, 33, PCD8544_PIXEL_BLACK, false);
    pcd8544_draw_rectagle(53, 23, 73, 33, PCD8544_PIXEL_BLACK, true);
    pcd8544_flush();
    vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS));
}

void draw_circle_demo(void) {
    ESP_LOGI(TAG, "Running draw circle demo");
    pcd8544_clear();
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, "Demo circle");
    pcd8544_draw_circle(21, 28, 10, PCD8544_PIXEL_BLACK, false);
    pcd8544_draw_circle(63, 28, 10, PCD8544_PIXEL_BLACK, true);
    pcd8544_flush();
    vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS));
}

void invert_color_demo(void) {
    ESP_LOGI(TAG, "Running invert color demo");
    pcd8544_clear();
    pcd8544_goto_xy(10, 20);
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, "Demo invert");
    pcd8544_flush();

    for (uint8_t i = 0; i < 8; i++) {
        pcd8544_invert(i % 2);
        vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS / 4));
    }

    pcd8544_invert(false);
}

void scroll_demo(void) {
    ESP_LOGI(TAG, "Running scroll demo");
    pcd8544_clear();
    pcd8544_goto_xy(5, 15);
    pcd8544_puts(PCD8544_FONT_5x7, PCD8544_PIXEL_BLACK, "Demo scroll");
    pcd8544_flush();

    for (uint8_t i = 0; i < 10; i++) {
        pcd8544_scroll(1, 0);
        vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS / 40));
    }

    for (uint8_t i = 0; i < 10; i++) {
        pcd8544_scroll(0, 1);
        vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS / 40));
    }

    for (uint8_t i = 0; i < 10; i++) {
        pcd8544_scroll(-1, 0);
        vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS / 40));
    }

    for (uint8_t i = 0; i < 10; i++) {
        pcd8544_scroll(0, -1);
        vTaskDelay(pdMS_TO_TICKS(DEMO_TIME_MS / 40));
    }
}

static void lcd_func_demo(void* arg) {
    pcd8544_set_backlight_fade(100, 2000, true);

    draw_bitmap_demo();
    draw_string_demo();
    draw_pixel_demo();
    draw_line_demo();
    draw_rectangle_demo();
    draw_circle_demo();
    invert_color_demo();
    scroll_demo();

    pcd8544_clear();
    pcd8544_set_backlight_fade(0, 2000, true);
    pcd8544_deinit();
    vTaskDelete(NULL);
}

void app_main(void) {
    /* Initialize the SPI bus */
    spi_bus_config_t buscfg = {
        .miso_io_num     = -1,  // Not used
        .mosi_io_num     = CONFIG_DIN_PIN_NUM,
        .sclk_io_num     = CONFIG_CLK_PIN_NUM,
        .quadwp_io_num   = -1,  // Not used
        .quadhd_io_num   = -1,  // Not used
        .max_transfer_sz = PARALLEL_LINES * PCD8544_H_RES_MAX * 2 + 8,
    };

    ESP_ERROR_CHECK(
        spi_bus_initialize(CONFIG_LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    /* Initialize LCD */
    pcd8544_io_config_t lcd_io_cfg = {
        .rst_gpio_num = CONFIG_RST_PIN_NUM,
        .ce_gpio_num  = CONFIG_CE_PIN_NUM,
        .dc_gpio_num  = CONFIG_DC_PIN_NUM,
        .bkl_gpio_num = CONFIG_BKL_PIN_NUM,
    };

    pcd8544_init(CONFIG_LCD_HOST, &lcd_io_cfg);

    /* Create a task to demonstrate some LCD's basic functions */
    xTaskCreate(lcd_func_demo, "LCD demo", 4 * 1024, NULL, 10, NULL);
}
