#ifndef __PCD8544_H__
#define __PCD8544_H__

#include <stdbool.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// PCD8544 Commandset
// ------------------
// General commands
#define PCD8544_POWERDOWN           0x04
#define PCD8544_ENTRYMODE           0x02
#define PCD8544_EXTENDEDINSTRUCTION 0x01
#define PCD8544_DISPLAYBLANK        0x00
#define PCD8544_DISPLAYNORMAL       0x04
#define PCD8544_DISPLAYALLON        0x01
#define PCD8544_DISPLAYINVERTED     0x05
// Normal instruction set
#define PCD8544_FUNCTIONSET         0x20
#define PCD8544_DISPLAYCONTROL      0x08
#define PCD8544_SETYADDR            0x40
#define PCD8544_SETXADDR            0x80
// Extended instruction set
#define PCD8544_SETTEMP             0x04
#define PCD8544_SETBIAS             0x10
#define PCD8544_SETVOP              0x80
// Display presets
#define PCD8544_LCD_BIAS            0x03  // Range: 0-7 (0x00-0x07)
#define PCD8544_LCD_TEMP            0x02  // Range: 0-3 (0x00-0x03)
#define PCD8544_LCD_CONTRAST        0x46  // Range: 0-127 (0x00-0x7F)

#define PCD8544_H_RES_MAX           84
#define PCD8544_V_RES_MAX           48
#define PCD8544_BUFFER_SIZE         ((PCD8544_H_RES_MAX * PCD8544_V_RES_MAX) / 8)

#define PCD8544_DEFAULT_CONTRAST    0x37

typedef enum {
    PCD8544_FONT_3x5,
    PCD8544_FONT_5x7,
} pcd8544_font_t;

typedef enum {
    PCD8544_PIXEL_WHITE,
    PCD8544_PIXEL_BLACK,
} pcd8544_pixel_color_t;

typedef enum {
    PCD8544_ALIGN_CENTER,
    PCD8544_ALIGN_LEFT,
    PCD8544_ALIGN_RIGHT,
    PCD8544_ALIGN_TOP,
    PCD8544_ALIGN_BOTTOM,
    PCD8544_ALIGN_TOP_LEFT,
    PCD8544_ALIGN_TOP_RIGHT,
    PCD8544_ALIGN_BOTTOM_LEFT,
    PCD8544_ALIGN_BOTTOM_RIGHT,
    PCD8544_ALIGN_MAX,
} pcd8544_obj_align_t;

typedef struct {
    int rst_gpio_num; /*!< GPIO used for resetting the display */
    int ce_gpio_num;  /*!< GPIO used for CE line */
    int dc_gpio_num;  /*!< GPIO used for DC (Data / Command) line */
    int bkl_gpio_num; /*!< GPIO used for backlight control */

    struct {
        uint8_t rst_active_high : 1; /*!< Display reset with logic level HIGH */
        uint8_t bkl_active_high : 1; /*!< Backlight ON with logic level HIGH */
        uint8_t                 : 6; /*!< Reserved */
    } flags;                         /*!< Extra flags to fine-tune the device */
} pcd8544_io_config_t;

esp_err_t pcd8544_init(const spi_host_device_t    spi_host,
                       const pcd8544_io_config_t* io_config);

esp_err_t pcd8544_deinit(void);

esp_err_t pcd8544_clear(void);

esp_err_t pcd8544_flush(void);

esp_err_t pcd8544_invert(bool invert);

esp_err_t pcd8544_set_contrast(uint8_t contrast);

esp_err_t pcd8544_set_backlight(uint8_t brightness);

esp_err_t pcd8544_set_backlight_fade(uint8_t brightness, int max_fade_time_ms,
                                     bool wait_fade_done);

esp_err_t pcd8544_goto_xy(uint8_t x, uint8_t y);

esp_err_t pcd8544_putc(pcd8544_font_t font, pcd8544_pixel_color_t color,
                       char c);

esp_err_t pcd8544_puts(pcd8544_font_t font, pcd8544_pixel_color_t color,
                       const char* format, ...)
    __attribute__((format(printf, 3, 4)));

esp_err_t pcd8544_draw_pixel(uint8_t x, uint8_t y, pcd8544_pixel_color_t color);

esp_err_t pcd8544_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                            pcd8544_pixel_color_t color);

esp_err_t pcd8544_draw_rectagle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                                pcd8544_pixel_color_t color, bool filled);

esp_err_t pcd8544_draw_circle(uint8_t x0, uint8_t y0, uint8_t r,
                              pcd8544_pixel_color_t color, bool filled);

esp_err_t pcd8544_scroll(int8_t dx, int8_t dy);

esp_err_t pcd8544_scroll_smooth(uint8_t dx, uint8_t dy, int max_scroll_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* __PCD8544_H__ */
