| Supported Target | ESP32 |
| ---------------- | ----- |

# PCD8544 Demo Example

## Overview

This example demonstrates basic usage of PCD8544 LCD driver.

## Pin Assignment

**Note:** The following pin assignments are used by default, you can change these in the `menuconfig`.

| LCD pin         | GPIO number |
| --------------- | ----------- |
| RST             | GPIO5       |
| CE              | GPIO18      |
| DC              | GPIO19      |
| DIN             | GPIO21      |
| CLK             | GPIO22      |
| BKL (backlight) | GPIO23      |

## Configure the project

Open the project configuration menu (`idf.py menuconfig`) then go into `Example Configuration` menu.

## Build and Flash

Enter `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type `Ctrl-]`.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

```
I (726) pcd8544: Successfully initialized
I (2376) pcd8544_demo: Running draw bitmap demo
I (4386) pcd8544_demo: Running string demo
I (6396) pcd8544_demo: Running draw pixel demo
I (8406) pcd8544_demo: Running draw line demo
I (10416) pcd8544_demo: Running draw rectangle demo
I (12426) pcd8544_demo: Running draw circle demo
I (14436) pcd8544_demo: Running invert color demo
I (18446) pcd8544_demo: Running scroll demo
I (22306) gpio: GPIO[18]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (22306) gpio: GPIO[5]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (22306) gpio: GPIO[23]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (22316) gpio: GPIO[19]| InputEn: 0| OutputEn: 0| OpenDrain: 0| Pullup: 1| Pulldown: 0| Intr:0
I (22326) pcd8544: Successfully deinitialized
```