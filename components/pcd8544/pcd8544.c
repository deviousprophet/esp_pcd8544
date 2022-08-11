#include "pcd8544.h"

#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "pcd8544_fonts.h"
#include "sys/param.h"

static const char* TAG = "pcd8544";

typedef struct {
    uint8_t                buffer[PCD8544_BUFFER_SIZE];
    uint8_t                update_xmin;
    uint8_t                update_xmax;
    uint8_t                update_ymin;
    uint8_t                update_ymax;
    uint8_t                _x;
    uint8_t                _y;
    bool                   is_inverted;
    ledc_channel_config_t* backlight_pwm;
    pcd8544_io_config_t*   io;
    spi_device_handle_t    spi_handle;
} pcd8544_handle_t;

pcd8544_handle_t* g_handle = NULL;

// This function is called (in irq context!) just before a transmission starts.
// It will set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t* t) {
    int dc = (int)t->user;
    gpio_set_level(g_handle->io->dc_gpio_num, dc);
}

esp_err_t pcd8544_send_cmd(uint8_t cmd) {
    spi_transaction_t t = {0};
    t.length            = 8;         // Command is 8 bits
    t.tx_buffer         = &cmd;      // Command
    t.user              = (void*)0;  // D/C needs to be set to 0
    return spi_device_polling_transmit(g_handle->spi_handle, &t);
}

esp_err_t pcd8544_send_data(uint8_t data) {
    spi_transaction_t t = {0};
    t.length            = 8;         // Data is 8 bits
    t.tx_buffer         = &data;     // Data
    t.user              = (void*)1;  // D/C needs to be set to 1
    return spi_device_polling_transmit(g_handle->spi_handle, &t);
}

void pcd8544_update_area(uint8_t xMin, uint8_t yMin, uint8_t xMax,
                         uint8_t yMax) {
    g_handle->update_xmin = MIN(xMin, g_handle->update_xmin);
    g_handle->update_ymin = MIN(yMin, g_handle->update_ymin);
    g_handle->update_xmax = MAX(xMax, g_handle->update_xmax);
    g_handle->update_ymax = MAX(yMax, g_handle->update_ymax);
}

esp_err_t pcd8544_reset(void) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    gpio_set_level(g_handle->io->rst_gpio_num,
                   g_handle->io->flags.rst_active_high);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(g_handle->io->rst_gpio_num,
                   1 - g_handle->io->flags.rst_active_high);
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

esp_err_t pcd8544_init(const spi_host_device_t    spi_host,
                       const pcd8544_io_config_t* io_config) {
    if (g_handle) return ESP_ERR_INVALID_STATE;

    if (!io_config) return ESP_ERR_INVALID_ARG;

    if (io_config->rst_gpio_num == -1) {
        ESP_LOGW(TAG, "Invalid RST gpio number");
        return ESP_ERR_INVALID_ARG;
    }

    if (io_config->dc_gpio_num == -1) {
        ESP_LOGW(TAG, "Invalid DC gpio number");
        return ESP_ERR_INVALID_ARG;
    }

    if (io_config->ce_gpio_num == -1) {
        ESP_LOGW(TAG, "Invalid CE gpio number");
        return ESP_ERR_INVALID_ARG;
    }

    g_handle = calloc(1, sizeof(pcd8544_handle_t));
    memset(g_handle, 0, sizeof(pcd8544_handle_t));
    g_handle->io = calloc(1, sizeof(pcd8544_io_config_t));
    memcpy(g_handle->io, io_config, sizeof(pcd8544_io_config_t));

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4 * 1000 * 1000,         // Clock 4MHz
        .mode           = 0,                       // SPI mode 0
        .spics_io_num   = io_config->ce_gpio_num,  // CE pin
        .queue_size     = 10,  // Queue 10 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback,  // Specify pre-transfer
                                                  // callback to handle D/C line
    };

    ESP_ERROR_CHECK(
        spi_bus_add_device(spi_host, &devcfg, &g_handle->spi_handle));

    gpio_set_direction(io_config->rst_gpio_num, GPIO_MODE_OUTPUT);
    gpio_set_direction(io_config->dc_gpio_num, GPIO_MODE_OUTPUT);

    if (io_config->bkl_gpio_num != -1) {
        ledc_timer_config_t ledc_timer = {
            .duty_resolution = LEDC_TIMER_13_BIT,     // resolution of PWM duty
            .freq_hz         = 5000,                  // frequency of PWM signal
            .speed_mode      = LEDC_HIGH_SPEED_MODE,  // timer mode
            .timer_num       = LEDC_TIMER_0,          // timer index
            .clk_cfg         = LEDC_AUTO_CLK,  // Auto select the source clock
        };
        // Set configuration of timer0 for high speed channels
        ledc_timer_config(&ledc_timer);

        g_handle->backlight_pwm = calloc(1, sizeof(ledc_channel_config_t));
        g_handle->backlight_pwm->channel    = LEDC_CHANNEL_0;
        g_handle->backlight_pwm->duty       = 0;
        g_handle->backlight_pwm->gpio_num   = io_config->bkl_gpio_num;
        g_handle->backlight_pwm->speed_mode = LEDC_HIGH_SPEED_MODE;
        g_handle->backlight_pwm->hpoint     = 0;
        g_handle->backlight_pwm->timer_sel  = LEDC_TIMER_0;
        g_handle->backlight_pwm->flags.output_invert =
            1 - io_config->flags.bkl_active_high;

        // Set LED Controller with previously prepared configuration
        ledc_channel_config(g_handle->backlight_pwm);

        // Initialize fade service
        ledc_fade_func_install(0);

    } else {
        ESP_LOGW(TAG, "Backlight is not used");
    }

    // Reset LCD
    pcd8544_reset();

    // // Go in extended mode
    // pcd8544_send_cmd(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION);

    // // LCD bias select
    // pcd8544_send_cmd(PCD8544_SETBIAS | 0x4);

    // // Set VOP
    // pcd8544_send_cmd(PCD8544_SETVOP | PCD8544_DEFAULT_CONTRAST);

    // Normal mode
    pcd8544_send_cmd(PCD8544_FUNCTIONSET);

    // Set display to Normal
    pcd8544_send_cmd(PCD8544_DISPLAYCONTROL | PCD8544_DISPLAYNORMAL);

    // Clear display
    pcd8544_clear();

    ESP_LOGI(TAG, "Successfully initialized");
    return ESP_OK;
}

esp_err_t pcd8544_deinit(void) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    // Reset LCD
    pcd8544_reset();

    spi_bus_remove_device(g_handle->spi_handle);

    gpio_reset_pin(g_handle->io->rst_gpio_num);
    gpio_reset_pin(g_handle->io->bkl_gpio_num);
    gpio_reset_pin(g_handle->io->dc_gpio_num);

    free(g_handle->backlight_pwm);
    free(g_handle->io);
    free(g_handle);

    ESP_LOGI(TAG, "Successfully deinitialized");
    return ESP_OK;
}

esp_err_t pcd8544_clear(void) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    pcd8544_goto_xy(0, 0);
    memset(g_handle->buffer, 0, PCD8544_BUFFER_SIZE);

    pcd8544_update_area(0, 0, PCD8544_H_RES_MAX - 1, PCD8544_V_RES_MAX - 1);
    pcd8544_flush();

    return ESP_OK;
}

esp_err_t pcd8544_flush(void) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    for (uint8_t i = 0; i < 6; i++) {
        // Not in range yet
        if (g_handle->update_ymin > ((i + 1) * 8)) continue;

        // Over range, stop
        if ((i * 8) > g_handle->update_ymax) break;

        pcd8544_send_cmd(PCD8544_SETYADDR | i);
        pcd8544_send_cmd(PCD8544_SETXADDR | g_handle->update_xmin);

        for (uint8_t j = g_handle->update_xmin; j <= g_handle->update_xmax; j++)
            pcd8544_send_data(g_handle->buffer[(i * PCD8544_H_RES_MAX) + j]);
    }

    g_handle->update_xmin = PCD8544_H_RES_MAX - 1;
    g_handle->update_xmax = 0;
    g_handle->update_ymin = PCD8544_V_RES_MAX - 1;
    g_handle->update_ymax = 0;

    return ESP_OK;
}

esp_err_t pcd8544_invert(bool invert) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    return pcd8544_send_cmd(
        PCD8544_DISPLAYCONTROL |
        (invert ? PCD8544_DISPLAYINVERTED : PCD8544_DISPLAYNORMAL));
}

esp_err_t pcd8544_is_inverted(bool* inverted) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;
    *inverted = g_handle->is_inverted;
    return ESP_OK;
}

esp_err_t pcd8544_set_contrast(uint8_t contrast) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    // Go in extended mode
    pcd8544_send_cmd(PCD8544_FUNCTIONSET | PCD8544_EXTENDEDINSTRUCTION);

    // Set VOP
    contrast = MIN(0, 0x7F);
    pcd8544_send_cmd(PCD8544_SETVOP | contrast);

    // Normal mode
    pcd8544_send_cmd(PCD8544_FUNCTIONSET);

    return ESP_OK;
}

esp_err_t pcd8544_set_backlight(uint8_t brightness) {
    esp_err_t err;

    err = ledc_set_duty(g_handle->backlight_pwm->speed_mode,
                        g_handle->backlight_pwm->channel,
                        (MIN(brightness, 100) * 8192) / 100);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "<%s> ledc_set_duty failed", esp_err_to_name(err));
        return err;
    }

    err = ledc_update_duty(g_handle->backlight_pwm->speed_mode,
                           g_handle->backlight_pwm->channel);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "<%s> ledc_update_duty failed", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t pcd8544_set_backlight_fade(uint8_t brightness, int max_fade_time_ms,
                                     bool wait_fade_done) {
    esp_err_t err;

    err = ledc_set_fade_with_time(
        g_handle->backlight_pwm->speed_mode, g_handle->backlight_pwm->channel,
        (MIN(brightness, 100) * 8192) / 100, max_fade_time_ms);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "<%s> ledc_set_fade_with_time failed",
                 esp_err_to_name(err));
        return err;
    }

    err = ledc_fade_start(
        g_handle->backlight_pwm->speed_mode, g_handle->backlight_pwm->channel,
        wait_fade_done ? LEDC_FADE_WAIT_DONE : LEDC_FADE_NO_WAIT);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "<%s> ledc_fade_start failed", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t pcd8544_goto_xy(uint8_t x, uint8_t y) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;
    g_handle->_x = x;
    g_handle->_y = y;
    return ESP_OK;
}

esp_err_t pcd8544_putc(pcd8544_font_t font, pcd8544_pixel_color_t color,
                       char c) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    uint8_t c_height, c_width, b;

    if (font == PCD8544_FONT_3x5) {
        c_width  = PCD8544_CHAR3x5_WIDTH;
        c_height = PCD8544_CHAR3x5_HEIGHT;

    } else {
        c_width  = PCD8544_CHAR5x7_WIDTH;
        c_height = PCD8544_CHAR5x7_HEIGHT;
    }

    if ((g_handle->_x + c_width) > PCD8544_H_RES_MAX) {
        // If at the end of a line of display, go to new line and set x to 0
        // position
        g_handle->_y += c_height;
        g_handle->_x = 0;
    }

    for (uint8_t i = 0; i < c_width - 1; i++) {
        if (font == PCD8544_FONT_3x5) {
            b = pcd8544_3x5_charset[c - 32][i];
        } else {
            b = pcd8544_5x7_charset[c - 32][i];
        }

        // if (b == 0x00 && (c != 0 && c != 32)) continue;

        for (uint8_t j = 0; j < c_height; j++) {
            if ((b >> j) & 1)
                pcd8544_draw_pixel(g_handle->_x, (g_handle->_y + j), color);
        }

        g_handle->_x++;
    }

    g_handle->_x++;

    return ESP_OK;
}

esp_err_t pcd8544_puts(pcd8544_font_t font, pcd8544_pixel_color_t color,
                       const char* format, ...) {
    esp_err_t ret = ESP_OK;
    char      temp_str[84];
    va_list   arg;

    va_start(arg, format);
    vsnprintf(temp_str, 84, format, arg);

    for (uint8_t i = 0; i < strlen(temp_str); i++) {
        ret = pcd8544_putc(font, color, temp_str[i]);
        if (ret != ESP_OK) break;
    }

    va_end(arg);
    return ret;
}

esp_err_t pcd8544_draw_pixel(uint8_t x, uint8_t y,
                             pcd8544_pixel_color_t color) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    if (x >= PCD8544_H_RES_MAX || y >= PCD8544_V_RES_MAX)
        return ESP_ERR_INVALID_ARG;

    if (color == PCD8544_PIXEL_BLACK)
        g_handle->buffer[x + (y / 8) * PCD8544_H_RES_MAX] |= 1 << (y % 8);
    else
        g_handle->buffer[x + (y / 8) * PCD8544_H_RES_MAX] &= ~(1 << (y % 8));

    pcd8544_update_area(x, y, x, y);
    return ESP_OK;
}

esp_err_t pcd8544_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                            pcd8544_pixel_color_t color) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    uint8_t dx, dy, temp;

    if (x0 > x1) {
        temp = x1;
        x1   = x0;
        x0   = temp;
    }

    if (y0 > y1) {
        temp = y1;
        y1   = y0;
        y0   = temp;
    }

    dx = x1 - x0;
    dy = y1 - y0;

    if (dx == 0) {
        do {
            pcd8544_draw_pixel(x0, y0, color);
            y0++;
        } while (y1 >= y0);
        return ESP_OK;
    }

    if (dy == 0) {
        do {
            pcd8544_draw_pixel(x0, y0, color);
            x0++;
        } while (x1 >= x0);
        return ESP_OK;
    }

    /* Based on Bresenham's line algorithm  */
    if (dx > dy) {
        temp = 2 * dy - dx;
        while (x0 != x1) {
            pcd8544_draw_pixel(x0, y0, color);
            x0++;
            if (temp > 0) {
                y0++;
                temp += 2 * dy - 2 * dx;
            } else {
                temp += 2 * dy;
            }
        }
        pcd8544_draw_pixel(x0, y0, color);

    } else {
        temp = 2 * dx - dy;
        while (y0 != y1) {
            pcd8544_draw_pixel(x0, y0, color);
            y0++;
            if (temp > 0) {
                x0++;
                temp += 2 * dy - 2 * dx;
            } else {
                temp += 2 * dy;
            }
        }
        pcd8544_draw_pixel(x0, y0, color);
    }
    return ESP_OK;
}

esp_err_t pcd8544_draw_rectagle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                                pcd8544_pixel_color_t color, bool filled) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    if (filled) {
        for (; y0 <= y1; y0++) pcd8544_draw_line(x0, y0, x1, y0, color);

    } else {
        pcd8544_draw_line(x0, y0, x1, y0, color);  // Top
        pcd8544_draw_line(x0, y0, x0, y1, color);  // Left
        pcd8544_draw_line(x1, y0, x1, y1, color);  // Right
        pcd8544_draw_line(x0, y1, x1, y1, color);  // Bottom
    }

    return ESP_OK;
}

esp_err_t pcd8544_draw_circle(uint8_t x0, uint8_t y0, uint8_t r,
                              pcd8544_pixel_color_t color, bool filled) {
    if (!g_handle) return ESP_ERR_INVALID_STATE;

    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    pcd8544_draw_pixel(x0, y0 + r, color);
    pcd8544_draw_pixel(x0, y0 - r, color);
    pcd8544_draw_pixel(x0 + r, y0, color);
    pcd8544_draw_pixel(x0 - r, y0, color);

    if (filled)
        pcd8544_draw_line(MAX(x0 - r, 0), y0, MIN(x0 + r, PCD8544_H_RES_MAX),
                          y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        if (filled) {
            pcd8544_draw_line(MAX(x0 - x, 0), MIN(y0 + y, PCD8544_V_RES_MAX),
                              MIN(x0 + x, PCD8544_H_RES_MAX),
                              MIN(y0 + y, PCD8544_V_RES_MAX), color);
            pcd8544_draw_line(MIN(x0 + x, PCD8544_H_RES_MAX), MAX(y0 - y, 0),
                              MAX(x0 - x, 0), MAX(y0 - y, 0), color);

            pcd8544_draw_line(MIN(x0 + y, PCD8544_H_RES_MAX),
                              MIN(y0 + x, PCD8544_V_RES_MAX), MAX(x0 - y, 0),
                              MIN(y0 + x, PCD8544_V_RES_MAX), color);
            pcd8544_draw_line(MIN(x0 + y, PCD8544_H_RES_MAX), MAX(y0 - x, 0),
                              MAX(x0 - y, 0), MAX(y0 - x, 0), color);

        } else {
            pcd8544_draw_pixel(x0 + x, y0 + y, color);
            pcd8544_draw_pixel(x0 - x, y0 + y, color);
            pcd8544_draw_pixel(x0 + x, y0 - y, color);
            pcd8544_draw_pixel(x0 - x, y0 - y, color);

            pcd8544_draw_pixel(x0 + y, y0 + x, color);
            pcd8544_draw_pixel(x0 - y, y0 + x, color);
            pcd8544_draw_pixel(x0 + y, y0 - x, color);
            pcd8544_draw_pixel(x0 - y, y0 - x, color);
        }
    }

    return ESP_OK;
}

esp_err_t pcd8544_draw_bitmap(const uint8_t* bitmap) {
    memcpy(g_handle->buffer, bitmap, PCD8544_BUFFER_SIZE);
    pcd8544_update_area(0, 0, PCD8544_H_RES_MAX - 1, PCD8544_V_RES_MAX - 1);
    return ESP_OK;
}

esp_err_t pcd8544_scroll(int8_t dx, int8_t dy) {
    uint8_t temp_buffer[PCD8544_BUFFER_SIZE];
    memcpy(temp_buffer, g_handle->buffer, PCD8544_BUFFER_SIZE);
    memset(g_handle->buffer, 0, PCD8544_BUFFER_SIZE);

    for (uint8_t x = 0; x < PCD8544_H_RES_MAX; x++) {
        for (uint8_t y = 0; y < PCD8544_V_RES_MAX; y++) {
            if (temp_buffer[x + (y / 8) * PCD8544_H_RES_MAX] & (1 << y % 8)) {
                uint8_t new_x = x + dx;
                uint8_t new_y = y + dy;
                pcd8544_draw_pixel(new_x, new_y, PCD8544_PIXEL_BLACK);
            }
        }
    }

    pcd8544_update_area(0, 0, PCD8544_H_RES_MAX - 1, PCD8544_V_RES_MAX - 1);
    pcd8544_flush();
    return ESP_OK;
}

esp_err_t pcd8544_scroll_smooth(uint8_t dx, uint8_t dy,
                                int max_scroll_time_ms) {
    // TODO: smooth scrolling animation
    return ESP_OK;
}