#pragma once

SemaphoreHandle_t lvglSemaphore = NULL;

#include <lvgl.h>
// #define DIRECT_MODE // Uncomment to enable full frame buffer
#define SCR_LOAD_ANIM_TIME 300 // ms

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;

long last_pressed = 0;

lv_obj_t *ui_Screen[16];
int8_t cur_screen_idx = 0;
int8_t screen_count = 1;

lv_obj_t *ui_Flex;

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf)
{
  Serial.printf(buf);
  Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
#ifndef DIRECT_MODE
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif
#endif // #ifndef DIRECT_MODE

  lv_disp_flush_ready(disp_drv);
}

void left_btn_pressed()
{
  if ((millis() - last_pressed) >= SCR_LOAD_ANIM_TIME)
  {
    cur_screen_idx--;
    if (cur_screen_idx < 0)
    {
      cur_screen_idx = screen_count - 1;
    }
    lv_scr_load_anim(ui_Screen[cur_screen_idx], LV_SCR_LOAD_ANIM_MOVE_TOP, SCR_LOAD_ANIM_TIME, 0, false);

    last_pressed = millis();
  }
}

void right_btn_pressed()
{
  if ((millis() - last_pressed) >= SCR_LOAD_ANIM_TIME)
  {
    cur_screen_idx++;
    if (cur_screen_idx >= screen_count)
    {
      cur_screen_idx = 0;
    }
    lv_scr_load_anim(ui_Screen[cur_screen_idx], LV_SCR_LOAD_ANIM_MOVE_BOTTOM, SCR_LOAD_ANIM_TIME, 0, false);

    last_pressed = millis();
  }
}

void lvgl_init()
{
  lv_init();

#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  screenWidth = gfx->width();
  screenHeight = gfx->height();

#ifdef DIRECT_MODE
  bufSize = screenWidth * screenHeight;
#else
  bufSize = screenWidth * 40;
#endif

#ifdef ESP32
#if defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL))
  disp_draw_buf = (lv_color_t *)gfx->getFramebuffer();
#else  // !(defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL)))
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf)
  {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
#endif // !(defined(DIRECT_MODE) && (defined(CANVAS) || defined(RGB_PANEL)))
#else  // !ESP32
  Serial.println("LVGL disp_draw_buf heap_caps_malloc failed! malloc again...");
  disp_draw_buf = (lv_color_t *)malloc(bufSize * 2);
#endif // !ESP32
  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  }
  else
  {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);

    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
#ifdef DIRECT_MODE
    disp_drv.direct_mode = true;
#endif
    lv_disp_drv_register(&disp_drv);
  }

  ui_Screen[0] = lv_obj_create(NULL);
  lv_obj_clear_flag(ui_Screen[0], LV_OBJ_FLAG_SCROLLABLE); /// Flags

  lv_obj_t *label = lv_label_create(ui_Screen[0]);
  lv_obj_set_size(label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_align(label, LV_ALIGN_TOP_MID);
  lv_obj_set_y(label, 4);
  lv_obj_set_style_text_font(label, LV_FONT_HEADER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(label, "LVGL Sensors Hub");

  ui_Flex = lv_obj_create(ui_Screen[0]);
  lv_obj_set_size(ui_Flex, gfx->width() - 20, gfx->height() - LV_HEADER_SIZE);
  lv_obj_center(ui_Flex);
  lv_obj_set_y(ui_Flex, (LV_HEADER_SIZE - 20) / 2);
  lv_obj_set_flex_flow(ui_Flex, LV_FLEX_FLOW_ROW_WRAP);

  lv_disp_load_scr(ui_Screen[0]);

  /* Init buttons */
  pinMode(LEFT_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(LEFT_BTN_PIN, left_btn_pressed, FALLING);
  pinMode(RIGHT_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(RIGHT_BTN_PIN, right_btn_pressed, FALLING);

  lvglSemaphore = xSemaphoreCreateMutex();
}

void lvgl_loop()
{
  if (xSemaphoreTake(lvglSemaphore, (TickType_t)10) == pdTRUE)
  {
    lv_timer_handler(); /* let the GUI do its work */
    xSemaphoreGive(lvglSemaphore);
  }

#ifdef DIRECT_MODE
#if defined(CANVAS) || defined(RGB_PANEL)
  gfx->flush();
#else // !(defined(CANVAS) || defined(RGB_PANEL))
#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#else
  gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
#endif
#endif // !(defined(CANVAS) || defined(RGB_PANEL))
#else  // !DIRECT_MODE
#ifdef CANVAS
  gfx->flush();
#endif
#endif // !DIRECT_MODE
}