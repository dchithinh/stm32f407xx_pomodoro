#include <stdint.h>
#include "lvgl.h"
#include "settings_screen.h"

static lv_obj_t *fullscreen_timer_cont = NULL;
static lv_obj_t *fullscreen_timer_label = NULL;

static void ui_full_screen_set_bg_by_theme(lv_obj_t *parent);
static void ui_full_screen_fade_in_obj(lv_obj_t *obj, uint32_t duration_ms);
static void ui_full_screen_fade_out_obj(lv_obj_t *obj, uint32_t duration_ms);

void show_fullscreen_timer(lv_obj_t *parent)
{
    if (fullscreen_timer_cont) return; // Already shown

    fullscreen_timer_cont = lv_obj_create(parent);
    ui_full_screen_set_bg_by_theme(fullscreen_timer_cont);
    ui_full_screen_fade_in_obj(fullscreen_timer_cont, 2000); // Fade in over 2 seconds

    fullscreen_timer_label = lv_label_create(fullscreen_timer_cont);
    lv_obj_center(fullscreen_timer_label);
    lv_obj_set_style_text_font(fullscreen_timer_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(fullscreen_timer_label, "00:00");
    lv_obj_set_style_text_color(fullscreen_timer_label, lv_color_hex(0x008080), 0);
    lv_obj_move_foreground(fullscreen_timer_cont);
}

void update_fullscreen_timer(uint32_t remaining)
{
    remaining /= 1000; // Convert ms to seconds
    if (!fullscreen_timer_label) return;
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%02d:%02d", remaining / 60, remaining % 60);
    lv_label_set_text(fullscreen_timer_label, buf);
}

void hide_fullscreen_timer(void)
{
    if (fullscreen_timer_cont) {
        lv_obj_del(fullscreen_timer_cont);
        fullscreen_timer_cont = NULL;
        fullscreen_timer_label = NULL;
    }
}

static void ui_full_screen_set_bg_by_theme(lv_obj_t *parent)
{
    if (ui_get_theme() == POMO_DARK_THEME) {
        lv_obj_remove_style_all(parent);
        lv_obj_set_size(parent, LV_PCT(100), LV_PCT(100));
        // Set background color for the parent/screen
        lv_obj_set_style_bg_color(parent, lv_color_hex(0x343247), 0);
        lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_all(parent, 0, 0);
        lv_obj_set_style_margin_all(parent, 0, 0);
        lv_obj_set_style_border_width(parent, 0, 0);

        // Remove scrollbars from parent
        lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    }
    else {
        lv_obj_set_style_bg_color(parent, lv_color_hex(0xffffff), 0);
    }
}

static void anim_set_opa_cb(void *var, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)var, value, LV_PART_MAIN);
}

static void ui_full_screen_fade_in_obj(lv_obj_t *obj, uint32_t duration_ms)
{
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, anim_set_opa_cb);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_start(&a);
}

static void ui_full_screen_fade_out_obj(lv_obj_t *obj, uint32_t duration_ms)
{
    lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&a, anim_set_opa_cb);
    lv_anim_set_time(&a, duration_ms);
    lv_anim_start(&a);
}