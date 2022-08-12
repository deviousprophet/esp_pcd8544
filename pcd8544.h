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

typedef enum {
    PCD8544_FONT_3x5, /*!< Font 3x5 */
    PCD8544_FONT_5x7, /*!< Font 5x7 */
} pcd8544_font_t;

typedef enum {
    PCD8544_PIXEL_WHITE, /*!< Pixel color white */
    PCD8544_PIXEL_BLACK, /*!< Pixel color black */
} pcd8544_pixel_color_t;

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

/**
 * @brief Initialize the display and enter into normal mode.
 *
 * @param[in] spi_host The SPI host used for LCD.
 *
 * @param[in] io_config Pointer of LCD gpio configuration.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if the display has already initialized.
 *      - ESP_ERR_INVALID_ARG if parameter is invalid.
 */
esp_err_t pcd8544_init(const spi_host_device_t    spi_host,
                       const pcd8544_io_config_t* io_config);

/**
 * @brief Deinitialize the display.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_deinit(void);

/**
 * @brief Clear the display.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_clear(void);

/**
 * @brief Update the display by flushing buffer.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_flush(void);

/**
 * @brief Set display invert control.
 *
 * @param[in] invert Whether to invert the display color.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_invert(bool invert);

/**
 * @brief Check if the display is inverted.
 *
 * @param[out] inverted Pointer of the output result.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_is_inverted(bool* inverted);

/**
 * @brief Set the contrast level.
 *
 * @param[in] contrast Contrast value to be set (range: 0x00 ~ 0x7F).
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_set_contrast(uint8_t contrast);

/**
 * @brief Set backlight brightness.
 *
 * @param[in] brightness Brightness percentage (range: 0 ~ 100).
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_set_backlight(uint8_t brightness);

/**
 * @brief Backlight fade function, with a limited time.
 *
 * @param[in] brightness Brightness percentage (range 0 ~ 100)
 *
 * @param[in] max_fade_time_ms The maximum time of the fading in miliseconds.
 *
 * @param[in] wait_fade_done Whether to block until fading done.
 *
 * @return
 */
esp_err_t pcd8544_set_backlight_fade(uint8_t brightness, int max_fade_time_ms,
                                     bool wait_fade_done);

/**
 * @brief Set the cursor coordinate.
 *
 * @note The origin of coordinates (x = 0, y = 0) is at the display's top left.
 *
 * @param[in] x X-coordinates (horizontal lines).
 *
 * @param[in] y Y-coordinates (vertical lines).
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_goto_xy(uint8_t x, uint8_t y);

/**
 * @brief Draw a character into the buffer.
 *
 * @param[in] font Font size.
 *
 * @param[in] color Pixel color.
 *
 * @param[in] c The character value.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_putc(pcd8544_font_t font, pcd8544_pixel_color_t color,
                       char c);

/**
 * @brief Draw a string into the buffer.
 *
 * @param[in] font Font size.
 *
 * @param[in] color Pixel color.
 *
 * @param[in] format Format of the string. See printf().
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_puts(pcd8544_font_t font, pcd8544_pixel_color_t color,
                       const char* format, ...)
    __attribute__((format(printf, 3, 4)));

/**
 * @brief Draw a pixel into the buffer.
 *
 * @param[in] x X-coordinates (horizontal lines).
 *
 * @param[in] y Y-coordinates (vertical lines).
 *
 * @param[in] color Pixel color.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_ARG if the coordinates is out of display resolution.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_draw_pixel(uint8_t x, uint8_t y, pcd8544_pixel_color_t color);

/**
 * @brief Draw a line into the buffer.
 *
 * @param[in] x0 The start X-coordinates (horizontal lines).
 *
 * @param[in] y0 The start Y-coordinates (vertical lines).
 *
 * @param[in] x1 The end X-coordinates (horizontal lines).
 *
 * @param[in] y1 The end Y-coordinates (vertical lines).
 *
 * @param[in] color Pixel color.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                            pcd8544_pixel_color_t color);

/**
 * @brief Draw a rectangle into the buffer.
 *
 * @param[in] x0 The start X-coordinates (horizontal lines).
 *
 * @param[in] y0 The start Y-coordinates (vertical lines).
 *
 * @param[in] x1 The end X-coordinates (horizontal lines).
 *
 * @param[in] y1 The end Y-coordinates (vertical lines).
 *
 * @param[in] color Pixel color.
 *
 * @param[in] filled Whether to fill the shape.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_draw_rectagle(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                                pcd8544_pixel_color_t color, bool filled);

/**
 * @brief Draw a circle into the buffer.
 *
 * @param[in] x0 Circle center X-coordinates (horizontal lines).
 *
 * @param[in] y0 Circle center Y-coordinates (vertical lines).
 *
 * @param[in] r Circle radius in pixels.
 *
 * @param[in] color Pixel color.
 *
 * @param[in] filled Whether to fill the shape.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_draw_circle(uint8_t x0, uint8_t y0, uint8_t r,
                              pcd8544_pixel_color_t color, bool filled);

/**
 * @brief Draw a bitmap image into the buffer.
 *
 * @note 84 x 48 pixels bitmap image buffer is recommended.
 *
 * @param[in] bitmap The bitmap image buffer.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_draw_bitmap(const uint8_t* bitmap);

/**
 * @brief Scroll the display by creating a new buffer and moving each pixel.
 *
 * @param[in] dx The x offset, can be negative to scroll backwards.
 *
 * @param[in] dy The y offset, can be negative to scroll backwards.
 *
 * @return
 *      - ESP_OK on success.
 *      - ESP_ERR_INVALID_STATE if:
 *              1. The display has already deinitialized.
 *              2. The display was not initialized yet.
 */
esp_err_t pcd8544_scroll(int8_t dx, int8_t dy);

// /**
//  * @brief
//  *
//  * @param[in] dx
//  *
//  * @param[in] dy
//  *
//  * @param[in] max_scroll_time_ms
//  *
//  * @return
//  */
// esp_err_t pcd8544_scroll_smooth(uint8_t dx, uint8_t dy, int
// max_scroll_time_ms);

#ifdef __cplusplus
}
#endif

#endif /* __PCD8544_H__ */
