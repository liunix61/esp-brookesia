/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include "esp_brookesia.hpp"
/* These are built-in app examples in `esp-brookesia` library */
#include "app_examples/phone/simple_conf/src/phone_app_simple_conf.hpp"
#include "app_examples/phone/complex_conf/src/phone_app_complex_conf.hpp"
#include "app_examples/phone/squareline/src/phone_app_squareline.hpp"

#define EXAMPLE_SHOW_MEM_INFO             (1)

static const char *TAG = "app_main";

static void on_clock_update_timer_cb(struct _lv_timer_t *t);

extern "C" void app_main(void)
{
    bsp_i2c_init();
    lv_disp_t *disp = bsp_display_start();

    ESP_LOGI(TAG, "Display ESP-Brookesia phone demo");
    /**
     * To avoid errors caused by multiple tasks simultaneously accessing LVGL,
     * should acquire a lock before operating on LVGL.
     */
    bsp_display_lock(0);

    /* Create a phone object */
    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone(disp);
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone, "Create phone failed");

    /* Try using a stylesheet that corresponds to the resolution */
    ESP_Brookesia_PhoneStylesheet_t *phone_stylesheet = nullptr;
    if ((BSP_LCD_H_RES == 480) && (BSP_LCD_V_RES == 480)) {
        phone_stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_480_480_DARK_STYLESHEET();
        ESP_BROOKESIA_CHECK_NULL_EXIT(phone_stylesheet, "Create phone stylesheet failed");
    } else if ((BSP_LCD_H_RES == 800) && (BSP_LCD_V_RES == 480)) {
        phone_stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_800_480_DARK_STYLESHEET();
        ESP_BROOKESIA_CHECK_NULL_EXIT(phone_stylesheet, "Create phone stylesheet failed");
    }
    if (phone_stylesheet != nullptr) {
        ESP_LOGI(TAG, "Using stylesheet (%s)", phone_stylesheet->core.name);
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->addStylesheet(phone_stylesheet), "Add phone stylesheet failed");
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->activateStylesheet(phone_stylesheet), "Activate phone stylesheet failed");
        delete phone_stylesheet;
    }

    /* Configure and begin the phone */
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->setTouchDevice(bsp_display_get_input_dev()), "Set touch device failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");
    // ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->getCoreHome().showContainerBorder(), "Show container border failed");

    /* Install apps */
    PhoneAppSimpleConf *phone_app_simple_conf = new PhoneAppSimpleConf(true, true);
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone_app_simple_conf, "Create phone app simple conf failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(phone_app_simple_conf) >= 0), "Install phone app simple conf failed");
    PhoneAppComplexConf *phone_app_complex_conf = new PhoneAppComplexConf(true, true);
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone_app_complex_conf, "Create phone app complex conf failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(phone_app_complex_conf) >= 0), "Install phone app complex conf failed");
    PhoneAppSquareline *phone_app_squareline = new PhoneAppSquareline(true, true);
    ESP_BROOKESIA_CHECK_NULL_EXIT(phone_app_squareline, "Create phone app squareline failed");
    ESP_BROOKESIA_CHECK_FALSE_EXIT((phone->installApp(phone_app_squareline) >= 0), "Install phone app squareline failed");

    /* Create a timer to update the clock */
    ESP_BROOKESIA_CHECK_NULL_EXIT(lv_timer_create(on_clock_update_timer_cb, 1000, phone), "Create clock update timer failed");

    /* Release the lock */
    bsp_display_unlock();

#if EXAMPLE_SHOW_MEM_INFO
    char buffer[128];    /* Make sure buffer is enough for `sprintf` */
    size_t internal_free = 0;
    size_t internal_total = 0;
    size_t external_free = 0;
    size_t external_total = 0;
    while (1) {
        internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        external_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        external_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        sprintf(buffer, "   Biggest /     Free /    Total\n"
                "\t  SRAM : [%8d / %8d / %8d]\n"
                "\t PSRAM : [%8d / %8d / %8d]",
                heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), internal_free, internal_total,
                heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM), external_free, external_total);
        ESP_LOGI(TAG, "%s", buffer);

        bsp_display_lock(0);
        // Update memory label on "Recents Screen"
        if (!phone->getHome().getRecentsScreen()->setMemoryLabel(internal_free / 1024, internal_total / 1024,
                external_free / 1024, external_total / 1024)) {
            ESP_LOGE(TAG, "Set memory label failed");
        }
        bsp_display_unlock();

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
#endif
}

static void on_clock_update_timer_cb(struct _lv_timer_t *t)
{
    time_t now;
    struct tm timeinfo;
    bool is_time_pm = false;
    ESP_Brookesia_Phone *phone = (ESP_Brookesia_Phone *)t->user_data;

    time(&now);
    localtime_r(&now, &timeinfo);
    is_time_pm = (timeinfo.tm_hour >= 12);

    // Update clock on "Status Bar"
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->getHome().getStatusBar()->setClock(timeinfo.tm_hour, timeinfo.tm_min, is_time_pm),
                                   "Refresh status bar failed");
}