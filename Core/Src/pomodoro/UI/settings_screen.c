#include <stdio.h>
#include <stdlib.h>
#include "settings_screen.h"
#include "event.h"
#include "lvgl.h"

typedef struct {
    lv_obj_t *work_roller;
    lv_obj_t *short_roller;
    lv_obj_t *long_roller;
    lv_obj_t *cycle_roller;
} rollers_t;

static PomodoroSettings_t settings = {
    .work_min = 25,
    .short_break_min = 5,
    .long_break_min = 15,
    .cycles_before_long = 4
};

static lv_obj_t *settings_screen;
static lv_style_t setting_section_label_style;
static lv_style_t setting_label_style;
static lv_style_t setting_section_style;
static lv_style_t setting_cont_style;
static lv_style_t btn_style;

static void ui_setting_screen_set_bg_by_theme(lv_obj_t *parent);
static void ui_setting_screen_init(void);

static void ui_setting_screen_init()
{
    lv_style_init(&setting_section_label_style);
    lv_style_set_text_font(&setting_section_label_style, &lv_font_montserrat_22);
    lv_style_set_text_color(&setting_section_label_style, lv_color_hex(0x4169E1));
    lv_style_init(&btn_style);

    lv_style_init(&setting_section_style);
    lv_style_set_size(&setting_section_style, LV_PCT(100), LV_SIZE_CONTENT);
    lv_style_set_bg_color(&setting_section_style, lv_color_hex(0xD3D3D3));
    lv_style_set_border_width(&setting_section_style, 0);
    lv_style_set_radius(&setting_section_style, 0);
    lv_style_set_bg_opa(&setting_section_style, LV_OPA_10);
    lv_style_set_radius(&setting_section_style, 8);

    lv_style_init(&setting_label_style);
    lv_style_set_text_font(&setting_label_style, &lv_font_montserrat_14);
    lv_style_set_text_color(&setting_label_style, lv_color_hex(0xADD8E6));

    lv_style_init(&setting_cont_style);
    lv_style_set_pad_all(&setting_cont_style, 0);
    lv_style_set_size(&setting_cont_style, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_style_set_flex_flow(&setting_cont_style, LV_FLEX_FLOW_COLUMN);
    lv_style_set_pad_row(&setting_cont_style, 6);

    lv_style_set_bg_color(&btn_style, lv_color_hex(0x2563EB));
    lv_style_set_bg_grad_color(&btn_style, lv_color_hex(0x1E40AF));
    lv_style_set_bg_grad_dir(&btn_style, LV_GRAD_DIR_VER);
    lv_style_set_text_color(&btn_style, lv_color_hex(0xEAEAEA));

    lv_style_set_shadow_width(&btn_style, 8);
    lv_style_set_shadow_color(&btn_style, lv_color_hex(0x1A1A26));
    lv_style_set_shadow_ofs_y(&btn_style, 4);
}

pomodoro_theme_e ui_get_theme(void) {
    //Only support Dark theme for now
    return POMO_DARK_THEME;
}

// Updated roller creation function
lv_obj_t *setting_screen_create_roller(lv_obj_t *parent, int min, int max, int initial_value, char *unit)
{
    static lv_style_t style_sel;
    static bool style_inited = false;

    if (initial_value < min || initial_value > max) {
        initial_value = min;
    }

    if (strlen(unit) + 2 + 1 + 1 > 7) {
        LV_LOG_WARN("Unit string too long, truncating to 3 chars.\n");
        unit[3] = '\0'; // Truncate to 3 characters
    }
    // Build options string
    static char opts[150]; // Each "xx min\n" can be up to 7 chars, so 25*7=175 is safe.
    char *p = opts;
    for (int i = min; i <= max; i++) {
        p += sprintf(p, "%d %s", i, unit);
        if (i != max) *p++ = '\n';
    }
    *p = '\0';

    lv_obj_t *roller = lv_roller_create(parent);
    lv_roller_set_options(roller, opts, LV_ROLLER_MODE_INFINITE);
    lv_roller_set_visible_row_count(roller, 2);

    /* IMPORTANT: If applying styles before lv_roller_set_options(),
        the styles will be overriden by roller default theme/styles*/
    if (!style_inited) {
        lv_style_init(&style_sel);
        lv_style_set_text_font(&style_sel, &lv_font_montserrat_14);
        lv_style_set_bg_color(&style_sel, lv_color_hex(0x808080));
        lv_style_set_border_width(&style_sel, 1);
        lv_style_set_border_color(&style_sel, lv_color_hex3(0xfff));

        style_inited = true;
    }

    lv_obj_set_width(roller, 100);
    lv_obj_add_style(roller, &style_sel, LV_PART_SELECTED);
    lv_obj_set_style_border_width(roller, 1, LV_PART_MAIN);
    lv_obj_set_width(roller, LV_SIZE_CONTENT);

    lv_obj_set_style_text_align(roller, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_bg_color(roller, lv_color_hex(0x6082B6 ), 0);
    lv_obj_set_style_bg_grad_color(roller, lv_color_hex(0x7393B3), 0);
    lv_obj_set_style_bg_grad_dir(roller, LV_GRAD_DIR_VER, 0);
    lv_obj_align(roller, LV_ALIGN_LEFT_MID, 10, 0);
    lv_roller_set_selected(roller, initial_value - min, LV_ANIM_OFF);

    return roller;
}

// Event handler to get all roller values
static void roller_event_handler(lv_event_t *e)
{
    rollers_t *rollers = lv_event_get_user_data(e);

    // Get selected indices (offset by min if needed)
    int work_idx = lv_roller_get_selected(rollers->work_roller);
    int short_idx = lv_roller_get_selected(rollers->short_roller);
    int long_idx = lv_roller_get_selected(rollers->long_roller);
    int cycle_idx = lv_roller_get_selected(rollers->cycle_roller);

    char buf[8];

    lv_roller_get_selected_str(rollers->work_roller, buf, sizeof(buf));
    settings.work_min = atoi(buf);

    lv_roller_get_selected_str(rollers->short_roller, buf, sizeof(buf));
    settings.short_break_min = atoi(buf);

    lv_roller_get_selected_str(rollers->long_roller, buf, sizeof(buf));
    settings.long_break_min = atoi(buf);

    lv_roller_get_selected_str(rollers->cycle_roller, buf, sizeof(buf));
    settings.cycles_before_long = atoi(buf);

    LV_LOG_USER("Work: %d, Short: %d, Long: %d, Cycle: %d\n", settings.work_min, settings.short_break_min, settings.long_break_min, settings.cycles_before_long);
}

static void setting_event_handler(lv_event_t *e)
{
    LV_LOG_USER("Settings saved. Returning to Main screen...\n");
    event_dispatch(EVENT_SETTINGS, &settings);

    ui_main_screen(lv_scr_act());

    if (settings_screen) {
        lv_obj_del(settings_screen);
        settings_screen = NULL;
    }
}

void show_settings_screen(lv_obj_t *parent)
{
    static rollers_t rollers;
    static lv_obj_t *cycle_setting;

    settings_screen = lv_obj_create(parent);
    ui_setting_screen_set_bg_by_theme(settings_screen);

    ui_setting_screen_init();

    lv_obj_t *timer_section = lv_obj_create(settings_screen);
    lv_obj_add_style(timer_section, &setting_section_style, 0);

    lv_obj_t *work_min_cont = lv_obj_create(timer_section);
    lv_obj_remove_style_all(work_min_cont);
    lv_obj_t *work_label = lv_label_create(work_min_cont);
    lv_label_set_text(work_label, "Pomodoro");
    lv_obj_add_style(work_label, &setting_label_style, 0);
    rollers.work_roller = setting_screen_create_roller(work_min_cont, 1, 25, settings.work_min, "min");
    lv_obj_set_flex_flow(work_min_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(work_min_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_style(work_min_cont, &setting_cont_style, 0);

    lv_obj_t *short_break_cont = lv_obj_create(timer_section);
    lv_obj_remove_style_all(short_break_cont);
    lv_obj_add_style(short_break_cont, &setting_cont_style, 0);
    lv_obj_t *short_break_label = lv_label_create(short_break_cont);
    lv_label_set_text(short_break_label, "Short Break");
    lv_obj_add_style(short_break_label, &setting_label_style, 0);
    rollers.short_roller = setting_screen_create_roller(short_break_cont, 1, 5, settings.short_break_min, "min");
    lv_obj_set_flex_flow(short_break_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(short_break_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *long_break_cont = lv_obj_create(timer_section);
    lv_obj_remove_style_all(long_break_cont);
    lv_obj_add_style(long_break_cont, &setting_cont_style, 0);
    lv_obj_t *long_break_label = lv_label_create(long_break_cont);
    lv_label_set_text(long_break_label, "Long Break");
    lv_obj_add_style(long_break_label, &setting_label_style, 0);
    rollers.long_roller = setting_screen_create_roller(long_break_cont, 1, 10, settings.long_break_min, "min");
    lv_obj_set_flex_flow(long_break_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(long_break_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t *cycle_cont = lv_obj_create(timer_section);
    lv_obj_remove_style_all(cycle_cont);
    lv_obj_add_style(cycle_cont, &setting_cont_style, 0);
    lv_obj_t *cycle_label = lv_label_create(cycle_cont);
    lv_label_set_text(cycle_label, "Cycles");
    lv_obj_add_style(cycle_label, &setting_label_style, 0);
    rollers.cycle_roller = setting_screen_create_roller(cycle_cont, 1, 4, settings.cycles_before_long, "");
    lv_obj_set_flex_flow(cycle_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cycle_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_set_flex_flow(timer_section, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(timer_section, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Attach the same handler to all rollers, passing the struct as user data
    lv_obj_add_event_cb(rollers.work_roller, roller_event_handler, LV_EVENT_VALUE_CHANGED, &rollers);
    lv_obj_add_event_cb(rollers.short_roller, roller_event_handler, LV_EVENT_VALUE_CHANGED, &rollers);
    lv_obj_add_event_cb(rollers.long_roller, roller_event_handler, LV_EVENT_VALUE_CHANGED, &rollers);
    lv_obj_add_event_cb(rollers.cycle_roller, roller_event_handler, LV_EVENT_VALUE_CHANGED, &rollers);


    lv_obj_t *btn_save = lv_btn_create(settings_screen);
    lv_obj_t* label_save = lv_label_create(btn_save);
    lv_label_set_text(label_save, "Save");
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_style(btn_save, &btn_style, 0);
    lv_obj_add_event_cb(btn_save, setting_event_handler, LV_EVENT_CLICKED, NULL);
}

static void ui_setting_screen_set_bg_by_theme(lv_obj_t *parent)
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

    }
    else {
        lv_obj_set_style_bg_color(parent, lv_color_hex(0xffffff), 0);
    }
}

int settings_get_work_time()
{
    return settings.work_min;
}

int settings_get_short_break()
{
    return settings.short_break_min;
}

int settings_get_long_break()
{
    return settings.long_break_min;
}

int settings_get_cycle_count()
{
   //TODO: Make this configurable in the future
   return 4; // Default cycle count
}