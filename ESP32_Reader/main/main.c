/* LwIP SNTP example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "rc522.h"
#include "DisplayUI.h"
#include "SNTP_local.h"

const int16_t SDA_PIN = GPIO_NUM_21;
const int16_t SCL_PIN = GPIO_NUM_22;

static const char* TAG = "RFID";
static uint8_t _Mode = 0;
volatile bool _ChangeUI = false;

char *TAG_SNTP;
char strftime_buf[64];
Time _now;

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */
// RTC_DATA_ATTR static int boot_count = 0;

void SplitData(void);
void task_SNTP(void);


void task_Display(void* pvParameters)
{
    SSD1306_t dev = *((SSD1306_t *)pvParameters);

    UI_ManualDisplay(&dev, _now, true);
    while (true) {
        if (_Mode == 0 && _ChangeUI) {
            UI_CheckingUser(&dev);
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            User _user;
            strcat(_user._Name, "Phong");
            strcat(_user._StudientID, "18520331");
            UI_HelloUser(&dev, _user);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            memset(_user._Name, 0, sizeof _user._Name);
            memset(_user._StudientID, 0, sizeof _user._StudientID);

            UI_LockCommand(&dev, false);
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            UI_LockCommand(&dev, true);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            _ChangeUI = false;
            UI_ManualDisplay(&dev, _now, true);
        }
        UI_ManualDisplay(&dev, _now, false);
    }
    vTaskDelete(NULL);
}

void tag_handler(uint8_t* sn) { // serial number is always 5 bytes long
    if (_Mode == 0){
        ESP_LOGI(TAG, "Tag: %#x %#x %#x %#x %#x",
                sn[0], sn[1], sn[2], sn[3], sn[4]
        );
        _ChangeUI = true;
    }
}

void app_main(void)
{
    SSD1306_t dev;

    UI_DisplayInit(&dev, SDA_PIN, SCL_PIN);

    xTaskCreate(&task_Display, "Manual", 4096, &dev, 5, NULL);

    const rc522_start_args_t start_args = {
        .miso_io  = 19,
        .mosi_io  = 23,
        .sck_io   = 18,
        .sda_io   = 5,
        .callback = &tag_handler,
    };
    rc522_start(start_args);

    while (true)
    {
        task_SNTP();
        SplitData();
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
