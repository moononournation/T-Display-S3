/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>

/*To use the built-in examples and demos of LVGL uncomment the includes below respectively.
 *You also need to copy `lvgl/examples` to `lvgl/src/examples`. Similarly for the demos `lvgl/demos` to `lvgl/src/demos`.
 Note that the `lv_examples` library is for LVGL v7 and you shouldn't install it for this version (since LVGL v8)
 as the examples and demos are now part of the main LVGL library. */

// #include <examples/lv_examples.h>
// #include <demos/lv_demos.h>

// #define DIRECT_MODE // Uncomment to enable full frame buffer

#include "T_Display_S3.h"

#define LEFT_BTN_PIN 0
#define RIGHT_BTN_PIN 14
#define SCR_LOAD_ANIM_TIME 300 // ms

uint32_t screenWidth;
uint32_t screenHeight;
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;

uint8_t current_screen = 1;
long last_pressed = 0;

lv_obj_t *ui_Screen1;
lv_obj_t *ui_Screen1_Chart1;
lv_chart_series_t *ser1;
lv_chart_series_t *ser2;
lv_obj_t *ui_Screen2;
lv_obj_t *ui_Screen2_Button1;
lv_obj_t *ui_Screen2_Button2;
lv_obj_t *ui_Screen2_Label1;
lv_obj_t *ui_Screen2_Label2;
char buf[16]; // sprintf text buffer
unsigned long next_readings_ms = 0;

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
    switch (current_screen)
    {
    case 1:
      lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_TOP, SCR_LOAD_ANIM_TIME, 0, false);
      current_screen = 2;
      break;
    default:
      lv_scr_load_anim(ui_Screen1, LV_SCR_LOAD_ANIM_MOVE_TOP, SCR_LOAD_ANIM_TIME, 0, false);
      current_screen = 1;
      break;
    }

    last_pressed = millis();
  }
}

void right_btn_pressed()
{
  if ((millis() - last_pressed) >= SCR_LOAD_ANIM_TIME)
  {
    switch (current_screen)
    {
    case 1:
      lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, SCR_LOAD_ANIM_TIME, 0, false);
      current_screen = 2;
      break;
    default:
      lv_scr_load_anim(ui_Screen1, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, SCR_LOAD_ANIM_TIME, 0, false);
      current_screen = 1;
      break;
    }

    last_pressed = millis();
  }
}

void setup()
{
  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX LVGL_Arduino_v8 example ");
  String LVGL_Arduino = String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.println(LVGL_Arduino);

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

    /* Option 1: Create simple label */
    // lv_obj_t *label = lv_label_create(lv_scr_act());
    // lv_label_set_text(label, "Hello Arduino! (V" GFX_STR(LVGL_VERSION_MAJOR) "." GFX_STR(LVGL_VERSION_MINOR) "." GFX_STR(LVGL_VERSION_PATCH) ")");
    // lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    ui_Screen1_Chart1 = lv_chart_create(ui_Screen1);
    lv_obj_set_size(ui_Screen1_Chart1, gfx->width() - 20, gfx->height() - 20);
    lv_obj_center(ui_Screen1_Chart1);
    lv_chart_set_type(ui_Screen1_Chart1, LV_CHART_TYPE_LINE); /*Show lines and points too*/

    /*Do not display points on the data*/
    lv_obj_set_style_size(ui_Screen1_Chart1, 0, LV_PART_INDICATOR);

    /*Add two data series*/
    ser1 = lv_chart_add_series(ui_Screen1_Chart1, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ser2 = lv_chart_add_series(ui_Screen1_Chart1, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_SECONDARY_Y);

    ui_Screen2 = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen2, LV_OBJ_FLAG_SCROLLABLE); /// Flags

    ui_Screen2_Button1 = lv_btn_create(ui_Screen2);
    lv_obj_set_size(ui_Screen2_Button1, (gfx->width() - 30) / 2, gfx->height() - 20);
    lv_obj_set_x(ui_Screen2_Button1, -((gfx->width() - 10) / 4));
    lv_obj_set_align(ui_Screen2_Button1, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Screen2_Button1, LV_OBJ_FLAG_SCROLL_ON_FOCUS); /// Flags
    lv_obj_clear_flag(ui_Screen2_Button1, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_style_bg_color(ui_Screen2_Button1, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen2_Button1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Screen2_Button2 = lv_btn_create(ui_Screen2);
    lv_obj_set_size(ui_Screen2_Button2, (gfx->width() - 30) / 2, gfx->height() - 20);
    lv_obj_set_x(ui_Screen2_Button2, ((gfx->width() - 10) / 4));
    lv_obj_set_align(ui_Screen2_Button2, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_Screen2_Button2, LV_OBJ_FLAG_SCROLL_ON_FOCUS); /// Flags
    lv_obj_clear_flag(ui_Screen2_Button2, LV_OBJ_FLAG_SCROLLABLE);    /// Flags
    lv_obj_set_style_bg_color(ui_Screen2_Button2, lv_palette_main(LV_PALETTE_GREEN), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Screen2_Button2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Screen2_Label1 = lv_label_create(ui_Screen2);
    lv_obj_set_size(ui_Screen2_Label1, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(ui_Screen2_Label1, LV_ALIGN_CENTER, -((gfx->width() - 10) / 4), 0);
    lv_obj_set_style_text_align(ui_Screen2_Label1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Screen2_Label1, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Screen2_Label1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Screen2_Label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Screen2_Label2 = lv_label_create(ui_Screen2);
    lv_obj_set_size(ui_Screen2_Label2, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(ui_Screen2_Label2, LV_ALIGN_CENTER, ((gfx->width() - 10) / 4), 0);
    lv_obj_set_style_text_align(ui_Screen2_Label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Screen2_Label2, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Screen2_Label2, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Screen2_Label2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_disp_load_scr(ui_Screen1);

    /* Option 2: Try an example. See all the examples
     * online: https://docs.lvgl.io/master/examples.html
     * source codes: https://github.com/lvgl/lvgl/tree/e7f88efa5853128bf871dde335c0ca8da9eb7731/examples */
    // lv_example_btn_1();

    /* Option 3: Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMOS_WIDGETS*/
    // lv_demo_widgets();
    // lv_demo_benchmark();
    // lv_demo_keypad_encoder();
    // lv_demo_music();
    // lv_demo_stress();
  }

  /* Init buttons */
  pinMode(LEFT_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(LEFT_BTN_PIN, left_btn_pressed, FALLING);
  pinMode(RIGHT_BTN_PIN, INPUT_PULLUP);
  attachInterrupt(RIGHT_BTN_PIN, right_btn_pressed, FALLING);

  Serial.println("Setup done");
}

void loop()
{
  if (millis() > next_readings_ms)
  {
    int val1 = random(100);
    int val2 = random(20);
    lv_chart_set_next_value(ui_Screen1_Chart1, ser1, val1);
    lv_chart_set_next_value(ui_Screen1_Chart1, ser2, val2);
    lv_chart_refresh(ui_Screen1_Chart1); /*Required after direct set*/

    sprintf(buf, "%d%%", val1);
    lv_label_set_text(ui_Screen2_Label1, buf);
    sprintf(buf, "%d%%", val2);
    lv_label_set_text(ui_Screen2_Label2, buf);

    next_readings_ms += 1000;
  }

  lv_timer_handler(); /* let the GUI do its work */

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

  delay(5);
}
