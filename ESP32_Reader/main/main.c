#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h" // generated by "make menuconfig"
#include "rc522.h"
#include "DisplayUI.h"

const int16_t SDA_PIN = GPIO_NUM_21;
const int16_t SCL_PIN = GPIO_NUM_22;

static const char* TAG = "RFID";
static uint8_t _Mode = 0;
volatile bool _ChangeUI = false;
volatile Time _now;

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
}
