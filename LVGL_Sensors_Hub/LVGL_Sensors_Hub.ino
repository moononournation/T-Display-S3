/*******************************************************************************
 * LVGL Sensors Hub
 *
 * Dependent libraries:
 * LVGL V8: https://github.com/lvgl/lvgl/tree/release/v8.4
 *
 * LVGL Configuration file:
 * Copy your_arduino_path/libraries/lvgl/lv_conf_template.h
 * to your_arduino_path/libraries/lv_conf.h
 *
 * In lv_conf.h around line 15, enable config file:
 * #if 1 // Set it to "1" to enable content
 *
 * Then find and set:
 * #define LV_COLOR_DEPTH     16
 * #define LV_TICK_CUSTOM     1
 * #define LV_SPRINTF_USE_FLOAT 1
 *
 * For SPI/parallel 8 display set color swap can be faster, parallel 16/RGB screen don't swap!
 * #define LV_COLOR_16_SWAP   1 // for SPI and parallel 8
 * #define LV_COLOR_16_SWAP   0 // for parallel 16 and RGB
 *
 * Customize font size:
 * #define LV_FONT_MONTSERRAT_12 1
 * #define LV_FONT_MONTSERRAT_14 1
 ******************************************************************************/
#define LV_HEADER_SIZE 32
#define LV_FONT_HEADER &lv_font_montserrat_12
#define LV_FONT_FIGURE &lv_font_montserrat_14

#include <Wire.h>

#include "T_Display_S3.h"
#include "lvgl_func.h"
#include "sensors_func.h"

void core0_task(void *pvParam)
{
  while (1) // infinite loop
  {
    lvgl_loop();

    vTaskDelay(pdMS_TO_TICKS(5));
  }
  vTaskDelete(NULL);
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while (!Serial);
  Serial.println("LVGL Seonsors Party");

  Wire.begin(I2C_SDA, I2C_SCL);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  lvgl_init();

  // Pin LVGL loop to Core 0
  xTaskCreatePinnedToCore(
      (TaskFunction_t)core0_task,
      (const char *const)"Core 0 task",
      (const uint32_t)4096,
      (void *const)NULL,
      (UBaseType_t)configMAX_PRIORITIES - 1,
      (TaskHandle_t *const)NULL,
      (const BaseType_t)0);

  Serial.println("Setup done");
}

void loop()
{
  sensors_loop();
}
