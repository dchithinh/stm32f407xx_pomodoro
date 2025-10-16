#include "lvgl.h"
#include "demo.h"
#include "lvgl.h"
#include "lvgl.h"

void demo_load_gif(lv_obj_t * parent)
{
  lv_obj_t * gif = lv_gif_create(parent);
  lv_gif_set_src(gif, "S:/png/run_target.gif");
  lv_obj_align(gif , LV_ALIGN_CENTER, 0, 0);
}

void demo_widget(lv_obj_t * parent)
{ 

  static int col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static int row_dsc[] = {
      LV_GRID_FR(1), //State
      LV_GRID_FR(2), //Timer + arc
      LV_GRID_FR(1), //Buttons 
      LV_GRID_FR(1), //Cycle
      LV_GRID_TEMPLATE_LAST
  };


  lv_obj_t * grid_cont = lv_obj_create(parent);
  lv_obj_set_size(grid_cont, LV_PCT(100), LV_PCT(100));
  lv_obj_set_grid_dsc_array(grid_cont, col_dsc, row_dsc);

  lv_obj_t *state_obj = lv_obj_create(grid_cont);
  lv_obj_set_grid_cell(state_obj, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

  lv_obj_t * state = lv_label_create(state_obj);
  lv_label_set_text(state, "WORK");
  lv_obj_align(state, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * timer_obj = lv_obj_create(grid_cont);
  lv_obj_set_grid_cell(timer_obj, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
  lv_obj_t * timer_count = lv_label_create(timer_obj);
  lv_label_set_text(timer_count, "00:00");
  lv_obj_align(timer_count, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * buttons = lv_obj_create(grid_cont);
  lv_obj_set_grid_cell(buttons, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);
  lv_obj_t * buttons_label = lv_label_create(buttons);
  lv_label_set_text(buttons_label, "Buttons");
  lv_obj_align(buttons_label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * cycle = lv_obj_create(grid_cont);
  lv_obj_set_grid_cell(cycle, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 3, 1);
  lv_obj_t * cycle_label = lv_label_create(cycle);
  lv_label_set_text(cycle_label, "Cycle");

}

lv_obj_t *screen1 = NULL;
lv_obj_t *screen2 = NULL;

// Custom exec callback for LVGL animation
void anim_set_opa_cb(void *var, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)var, value, LV_PART_MAIN);
}

void transis_cb(lv_event_t *e)
{
    LV_LOG_USER("Transitioning screens");
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_GESTURE) {
      LV_LOG_USER("Gesture detected\n");
    }

    lv_obj_clear_flag(screen2, LV_OBJ_FLAG_HIDDEN);
    
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, screen2);
    lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, anim_set_opa_cb);
    lv_anim_set_time(&a, 5000);
    lv_anim_start(&a);
    lv_obj_move_foreground(screen2);
}

void demo_screen_anim_transis(lv_obj_t *parent)
{
    screen1 = lv_obj_create(parent);
    lv_obj_add_flag(screen1, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_size(screen1, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen1, lv_color_hex(0x300000), 0);

    lv_obj_t *button1 = lv_btn_create(screen1);
    lv_obj_t* label1 = lv_label_create(button1);
    lv_label_set_text(label1, "Transition");
    lv_obj_center(button1);

    lv_obj_add_event_cb(button1, transis_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(screen1, transis_cb, LV_EVENT_GESTURE, NULL);


    screen2 = lv_obj_create(parent);
    lv_obj_set_size(screen2, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(screen2, lv_color_hex(0x005000), 0);
    lv_obj_add_flag(screen2, LV_OBJ_FLAG_HIDDEN);

}

void demo_load_img(lv_obj_t * parent)
{
  lv_obj_set_style_bg_color(parent, lv_color_hex(0x343247), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
  lv_obj_t* settings_icon_img = lv_img_create(parent);
  lv_img_set_src(settings_icon_img, "S:/png/get_ready_64x64.png");
  lv_obj_set_align(settings_icon_img, LV_ALIGN_CENTER);

  lv_obj_set_style_img_recolor(settings_icon_img, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_img_recolor_opa(settings_icon_img, LV_OPA_100, 0);
}