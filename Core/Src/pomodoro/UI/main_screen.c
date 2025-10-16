#include <stdio.h>

#include "lvgl.h"
#include "event.h"
#include "timer.h"
#include "pomodoro.h"
#include "settings_screen.h"
#include "main_screen.h"
#include "full_screen.h"

#define POMO_MOVE_TO_FULLSCREEN_SEC     10

static lv_obj_t *main_cont;

static lv_obj_t *label_mode;
static lv_obj_t *label_timer;
static lv_obj_t *label_cycle;
static lv_obj_t *progress;
static lv_obj_t *label_pause;

static lv_obj_t *btn_start;
static lv_obj_t *btn_reset;
static lv_obj_t *btn_setting;

static lv_style_t progress_main_style;
static lv_style_t progress_indic_style;
static lv_style_t font_style;
static lv_style_t btn_style;


static lv_obj_t *icon_mode_cont;
static lv_obj_t *ready_icon;
static lv_obj_t *work_race_icon;
static lv_obj_t *work_run_icon;
static lv_obj_t *work_speed_icon;
static lv_obj_t *short_break_icon;
static lv_obj_t *long_break_icon;
// static lv_obj_t *objective_icon;

static bool timer_running = false;
static int work_state_elapsed_sec = 0;
static bool fullscreen_timer_active = false;
static bool fullscreen_enable = false;

/* Forward declarations */
static void ui_main_screen_set_bg_by_theme(lv_obj_t *parent);
static void ui_main_screen_init_style_by_theme(void);
static void update_timer_label(uint32_t remaining_ms);

static void start_event_cb(lv_event_t *e);
static void reset_event_cb(lv_event_t *e);
static void setting_event_cb(lv_event_t *e);

static void pomodoro_state_changed(PomodoroState_e state);
static void timer_tick_cb(lv_timer_t * timer);
static void ui_tick_cb(uint32_t remaining);
static void ui_update_ctrl_button(PomodoroState_e state);
static void ui_update_state_text(PomodoroState_e state);
static void ui_update_cycle_counter(void);

/* --- UI Functions --- */

static void ui_main_screen_set_bg_by_theme(lv_obj_t *parent)
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

static void ui_main_screen_init_style_by_theme(void) {

    lv_style_init(&progress_main_style);
    lv_style_init(&progress_indic_style);
    lv_style_init(&font_style);
    lv_style_init(&btn_style);

    if (ui_get_theme() == POMO_DARK_THEME) {
        lv_style_set_width(&progress_main_style, 10);
        lv_style_set_arc_color(&progress_main_style, lv_color_hex(0x1E1E2A));
        // lv_style_set_arc_opa(&progress_main_style, LV_OPA_50);

        lv_style_set_width(&progress_indic_style, 10);
        lv_style_set_arc_color(&progress_indic_style, lv_color_hex(0x4A90E2));

        #ifdef SCREEN_SIZE_240x320
        lv_style_set_text_font(&font_style, &lv_font_montserrat_12);
        lv_style_set_text_color(&font_style, lv_color_hex(0xffffff));
        #else
        lv_style_set_text_font(&font_style, &lv_font_montserrat_14);
        lv_style_set_text_color(&font_style, lv_color_hex(0xffffff));
        #endif
        lv_style_set_bg_color(&btn_style, lv_color_hex(0x2563EB));
        lv_style_set_bg_grad_color(&btn_style, lv_color_hex(0x1E40AF));
        lv_style_set_bg_grad_dir(&btn_style, LV_GRAD_DIR_VER);
        lv_style_set_text_color(&btn_style, lv_color_hex(0xEAEAEA));

        lv_style_set_shadow_width(&btn_style, 8);
        lv_style_set_shadow_color(&btn_style, lv_color_hex(0x1A1A26));
        lv_style_set_shadow_ofs_y(&btn_style, 4);

        // Rounding
        lv_style_set_radius(&btn_style, 8);
    }
    else {
        //TODO: adjust colors for light theme
        lv_style_set_width(&progress_main_style, 20);
        lv_style_set_arc_color(&progress_main_style, lv_color_hex(0x00ff00));

        lv_style_set_width(&progress_indic_style, 20);
        lv_style_set_arc_color(&progress_indic_style, lv_color_hex(0xff0000));
    }
}

static void label_event_cb(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_PRESSED || code == LV_EVENT_FOCUSED || code == LV_EVENT_HOVER_OVER) {
        // Stop scrolling: disable long mode
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_DEFOCUSED || code == LV_EVENT_HOVER_LEAVE) {
        // Resume scrolling: enable scroll again
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    }
}

void ui_main_screen_update_mode_icon(PomodoroState_e curr_state) {

    // Hide all icons initially
    if (ready_icon)         lv_obj_add_flag(ready_icon, LV_OBJ_FLAG_HIDDEN);
    if (work_race_icon)     lv_obj_add_flag(work_race_icon, LV_OBJ_FLAG_HIDDEN);
    if (work_run_icon)      lv_obj_add_flag(work_run_icon, LV_OBJ_FLAG_HIDDEN);
    if (work_speed_icon)    lv_obj_add_flag(work_speed_icon, LV_OBJ_FLAG_HIDDEN);
    // if (objective_icon)     lv_obj_add_flag(objective_icon, LV_OBJ_FLAG_HIDDEN);
    if (short_break_icon)   lv_obj_add_flag(short_break_icon, LV_OBJ_FLAG_HIDDEN);
    if (long_break_icon)    lv_obj_add_flag(long_break_icon, LV_OBJ_FLAG_HIDDEN);

    switch (curr_state) {
        case POMODORO_IDLE:
            lv_obj_clear_flag(ready_icon, LV_OBJ_FLAG_HIDDEN);
            break;
        case POMODORO_WORK:
        case POMODORO_PAUSED_WORK:
            lv_obj_clear_flag(work_run_icon, LV_OBJ_FLAG_HIDDEN);
            // lv_obj_clear_flag(objective_icon, LV_OBJ_FLAG_HIDDEN);
            break;
        case POMODORO_SHORT_BREAK:
            lv_obj_clear_flag(short_break_icon, LV_OBJ_FLAG_HIDDEN);
            break;
        case POMODORO_LONG_BREAK:
            lv_obj_clear_flag(long_break_icon, LV_OBJ_FLAG_HIDDEN);
            break;
        case POMODORO_PAUSED_BREAK:
            if(pomodoro_get_pause_break_type() == POMODORO_SHORT_BREAK) {
                lv_obj_clear_flag(short_break_icon, LV_OBJ_FLAG_HIDDEN);
            }
            else {
                lv_obj_clear_flag(long_break_icon, LV_OBJ_FLAG_HIDDEN);
            }

        default:
            // Optionally hide all
            break;
    }
}

void ui_main_screen(lv_obj_t *parent)
{
    // Initialize systems
    event_init();
    timer_init();
    ui_main_screen_init_style_by_theme();

    // Register callbacks
    pomodoro_set_state_callback(pomodoro_state_changed);
    pomodoro_set_tick_callback(ui_tick_cb);

    // Create periodic timer check using LVGL
    lv_timer_create(timer_tick_cb, 1000, NULL);  // Check every 1000ms
    
    /* Grid: 6 rows, 1 column */
    static int col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static int row_dsc[] = {
        LV_GRID_FR(2),     // Mode
        LV_GRID_FR(5),     // Timer + bar
        LV_GRID_FR(1),     // Buttons
        LV_GRID_FR(1),     // Cycle status
        LV_GRID_FR(1),     // Quotes
        LV_GRID_TEMPLATE_LAST
    };

    main_cont = lv_obj_create(parent);
    ui_main_screen_set_bg_by_theme(main_cont);

    lv_obj_set_grid_dsc_array(main_cont, col_dsc, row_dsc);
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* Add some margin top/bottom */
    lv_obj_set_style_pad_top(main_cont, 12, 0);
    lv_obj_set_style_pad_bottom(main_cont, 12, 0);
    lv_obj_set_style_pad_row(main_cont, 8, 0);   // spacing between rows

    /* Mode */
#if 0 //Will be removed/replaced
    label_mode = lv_label_create(main_cont);
    lv_label_set_text(label_mode, "Focus");
    lv_obj_set_grid_cell(label_mode,
                         LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_style(label_mode, &font_style, 0);
    lv_obj_set_style_text_font(label_mode, &lv_font_montserrat_36, 0);
    lv_obj_set_style_text_color(label_mode, lv_color_hex(0x00ff88), 0);
    lv_obj_add_flag(label_mode, LV_OBJ_FLAG_HIDDEN);
#endif

    static lv_style_t icon_style;
    lv_style_init(&icon_style);
    lv_style_set_img_recolor(&icon_style, lv_color_hex(0x000000));
    lv_style_set_img_recolor_opa(&icon_style, LV_OPA_100);

    int img_zoom = 256;
    #ifdef SCREEN_SIZE_240x320
    img_zoom = 256 * 0.7;
    #endif

    icon_mode_cont = lv_obj_create(main_cont);
    lv_obj_remove_style_all(icon_mode_cont);
    lv_obj_set_size(icon_mode_cont, LV_PCT(80), LV_PCT(100));
    lv_obj_set_grid_cell(icon_mode_cont, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_set_flex_flow(icon_mode_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(icon_mode_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    //TODO: Should add main screen de-constructor for these deinit
    if (ready_icon)         lv_obj_del(ready_icon);
    if (work_speed_icon)    lv_obj_del(work_speed_icon);
    if (work_run_icon)      lv_obj_del(work_run_icon);
    if (work_race_icon)     lv_obj_del(work_race_icon);
    // if (objective_icon)     lv_obj_del(objective_icon);
    if (short_break_icon)   lv_obj_del(short_break_icon);
    if (long_break_icon)    lv_obj_del(long_break_icon);

    LV_IMG_DECLARE(get_ready_64x64);
    ready_icon = lv_img_create(icon_mode_cont);
    lv_obj_add_style(ready_icon, &icon_style, 0);
    // lv_img_set_src(ready_icon, "S:/png/get_ready_64x64.png");
    lv_img_set_src(ready_icon, &get_ready_64x64);
    lv_obj_set_style_img_recolor(ready_icon, lv_color_hex(0xBBBBBB), 0);
    lv_img_set_zoom(ready_icon, img_zoom);

    LV_IMG_DECLARE(run);
    work_run_icon = lv_img_create(icon_mode_cont);
    lv_obj_add_style(work_run_icon, &icon_style, 0);
    // lv_img_set_src(work_run_icon, "S:/png/run.png");
    lv_img_set_src(work_run_icon, &run);
    lv_obj_set_style_img_recolor(work_run_icon, lv_color_hex(0xBBBBBB), 0);
    lv_img_set_zoom(work_run_icon, img_zoom);

    LV_IMG_DECLARE(race);
    work_race_icon = lv_img_create(icon_mode_cont);
    lv_obj_add_style(work_race_icon, &icon_style, 0);
    // lv_img_set_src(work_race_icon, "S:/png/race.png");
    lv_img_set_src(work_race_icon, &race);
    lv_obj_set_style_img_recolor(work_race_icon, lv_color_hex(0xBBBBBB), 0);
    lv_img_set_zoom(work_race_icon, img_zoom);

    LV_IMG_DECLARE(speed);
    work_speed_icon = lv_img_create(icon_mode_cont);
    lv_obj_add_style(work_speed_icon, &icon_style, 0);
    // lv_img_set_src(work_speed_icon, "S:/png/speed.png");
    lv_img_set_src(work_speed_icon, &speed);
    lv_obj_set_style_img_recolor(work_speed_icon, lv_color_hex(0xBBBBBB), 0);
    lv_img_set_zoom(work_speed_icon, img_zoom);

    #if 0
    LV_IMG_DECLARE(objective);
    objective_icon = lv_img_create(icon_mode_cont);
    lv_obj_add_style(objective_icon, &icon_style, 0);
    lv_img_set_src(objective_icon, "S:/png/objective.png");
    lv_img_set_src(objective_icon, &objective);
    lv_obj_set_style_img_recolor(objective_icon, lv_color_hex(0xBBBBBB), 0);
    #endif

    LV_IMG_DECLARE(short_break);
    short_break_icon = lv_img_create(icon_mode_cont);
    lv_obj_add_style(short_break_icon, &icon_style, 0);
    // lv_img_set_src(short_break_icon, "S:/png/short_break.png");
    lv_img_set_src(short_break_icon, &short_break);
    lv_obj_set_style_img_recolor(short_break_icon, lv_color_hex(0xBBBBBB), 0);
    lv_img_set_zoom(short_break_icon, img_zoom);

    LV_IMG_DECLARE(long_break);
    long_break_icon = lv_img_create(icon_mode_cont);
    lv_obj_add_style(long_break_icon, &icon_style, 0);
    // lv_img_set_src(long_break_icon, "S:/png/long_break.png");
    lv_img_set_src(long_break_icon, &long_break);
    lv_obj_set_style_img_recolor(long_break_icon, lv_color_hex(0xBBBBBB), 0);
    lv_img_set_zoom(long_break_icon, img_zoom);

    ui_main_screen_update_mode_icon(POMODORO_IDLE);

    /* Timer row */
    lv_obj_t *timer_cont = lv_obj_create(main_cont);
    lv_obj_remove_style_all(timer_cont);
    lv_obj_clear_flag(timer_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_grid_cell(timer_cont, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    /* Circular progress indicator */
    progress = lv_arc_create(timer_cont);
    int32_t progress_w = 200;
    int32_t progress_h = 200;
    #ifdef SCREEN_SIZE_240x320
    progress_w = 130;
    progress_h = 130;
    #else
    progress_w = 200;
    progress_h = 200;
    #endif
    lv_obj_set_size(progress, progress_w, progress_h);
    lv_obj_center(progress);

    lv_obj_remove_style(progress, NULL, LV_PART_KNOB);   // Remove the knob
    lv_obj_clear_flag(progress, LV_OBJ_FLAG_CLICKABLE);  // Make it non-interactive

    lv_arc_set_range(progress, 0, pomodoro_get_remaining_sec());
    lv_arc_set_value(progress, pomodoro_get_remaining_sec());
    lv_arc_set_bg_angles(progress, 0, 360);

    // Set rotation to start from top
    lv_arc_set_rotation(progress, 270);  // Rotate so 0 degrees is at 12 o'clock
    lv_arc_set_mode(progress, LV_ARC_MODE_REVERSE);

    lv_obj_add_style(progress, &progress_main_style, LV_PART_MAIN);
    lv_obj_add_style(progress, &progress_indic_style, LV_PART_INDICATOR);

    /* Timer label - positioned in center of circle */
    label_timer = lv_label_create(progress);  // Create as child of arc for centering
    lv_label_set_text(label_timer, "25:00");
    lv_obj_set_style_text_color(label_timer, lv_color_hex(0x4A90E2), 0);
    lv_obj_set_style_text_font(label_timer, &lv_font_montserrat_28, 0);
    lv_obj_center(label_timer);  // Center within the arc

    label_pause = lv_label_create(timer_cont);
    lv_label_set_text(label_pause, "Paused>");
    lv_obj_add_style(label_pause, &font_style, 0);
    lv_obj_align_to(label_pause, label_timer, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_add_flag(label_pause, LV_OBJ_FLAG_HIDDEN);

    /* Buttons */
    lv_obj_t *btn_row = lv_obj_create(main_cont);
    lv_obj_remove_style_all(btn_row);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row,
                          LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_grid_cell(btn_row,
                         LV_GRID_ALIGN_STRETCH, 0, 1,
                         LV_GRID_ALIGN_STRETCH, 2, 1);

    btn_start = lv_btn_create(btn_row);
    int16_t btn_w = 15;
    int16_t btn_h = 80;
    #ifdef SCREEN_SIZE_240x320
    btn_w = 30;
    btn_h = 80;
    #endif

    lv_obj_set_size(btn_start, LV_PCT(btn_w), LV_PCT(btn_h));
    lv_obj_add_style(btn_start, &btn_style, 0);
    lv_obj_add_event_cb(btn_start, start_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_start = lv_label_create(btn_start);
    lv_label_set_text(label_start, "Start");
    lv_obj_add_style(label_start, &font_style, 0);
    lv_obj_center(label_start);

    btn_reset = lv_btn_create(btn_row);
    lv_obj_set_size(btn_reset, LV_PCT(btn_w), LV_PCT(btn_h));
    lv_obj_add_style(btn_reset, &btn_style, 0);
    lv_obj_add_event_cb(btn_reset, reset_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_reset = lv_label_create(btn_reset);
    lv_label_set_text(label_reset, "Stop");
    lv_obj_add_style(label_reset, &font_style, 0);
    lv_obj_center(label_reset);

    btn_setting = lv_btn_create(btn_row);
    lv_obj_set_size(btn_setting, LV_PCT(btn_w), LV_PCT(btn_h));
    lv_obj_add_style(btn_setting, &btn_style, 0);
    lv_obj_add_event_cb(btn_setting, setting_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label_setting = lv_label_create(btn_setting);
    lv_label_set_text(label_setting, "Settings");
    lv_obj_add_style(label_setting, &font_style, 0);
    lv_obj_center(label_setting);

    /* Cycle status */
    label_cycle = lv_label_create(main_cont);
    lv_label_set_text(label_cycle, "Cycle: 0 / 4");
    lv_obj_set_grid_cell(label_cycle,
                         LV_GRID_ALIGN_CENTER, 0, 1,
                         LV_GRID_ALIGN_CENTER, 3, 1);
    
    lv_obj_t *label_quote = lv_label_create(main_cont);
    lv_label_set_text(label_quote, "Focus on being productive instead of busy");
    lv_obj_set_grid_cell(label_quote, LV_GRID_ALIGN_CENTER, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_style_text_color(label_quote, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(label_quote, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(label_quote, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(label_quote, 200);
    lv_obj_add_flag(label_quote, LV_OBJ_FLAG_CLICKABLE);

    // Add event handler to pause/resume
    lv_obj_add_event_cb(label_quote, label_event_cb, LV_EVENT_ALL, NULL);


    /* Update initial timer display */
    update_timer_label(pomodoro_get_remaining_sec() * 1000);

    // Initialize to IDLE state
    pomodoro_state_changed(POMODORO_IDLE);
}

static void update_timer_label(uint32_t remaining_ms)
{
    uint32_t total_seconds = remaining_ms / 1000;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;

    static char buf[8];
    lv_snprintf(buf, sizeof(buf), "%02d:%02d", minutes, seconds);
    lv_label_set_text(label_timer, buf);

    // Update progress bar using seconds
    lv_arc_set_value(progress, total_seconds);
}

static void ui_update_ctrl_button(PomodoroState_e state)
{
    if(state == POMODORO_IDLE) {
        lv_obj_add_flag(btn_reset, LV_OBJ_FLAG_HIDDEN); // Hide Reset
        lv_obj_set_align(btn_start, LV_ALIGN_CENTER);   // Center Start button

        lv_label_set_text(lv_obj_get_child(btn_start, 0), "Start");
        lv_obj_add_flag(label_pause, LV_OBJ_FLAG_HIDDEN); // Hide "Paused" label
        lv_obj_clear_flag(btn_setting, LV_OBJ_FLAG_HIDDEN); // Show Settings when idle
    }
    else if (state == POMODORO_WORK ||
                state == POMODORO_SHORT_BREAK ||
                state == POMODORO_LONG_BREAK) {
        lv_label_set_text(lv_obj_get_child(btn_start, 0), "Pause");
        lv_obj_add_flag(label_pause, LV_OBJ_FLAG_HIDDEN); // Hide "Paused" label
        lv_obj_set_align(btn_reset, LV_ALIGN_RIGHT_MID);  // Align Reset to right
        lv_obj_clear_flag(btn_reset, LV_OBJ_FLAG_HIDDEN); // Show Reset
        lv_obj_add_flag(btn_setting, LV_OBJ_FLAG_HIDDEN); // Hide Settings when running
    }
    else if (state == POMODORO_PAUSED_WORK ||
            state == POMODORO_PAUSED_BREAK) {
        lv_label_set_text(lv_obj_get_child(btn_start, 0), "Resume");
        lv_obj_clear_flag(label_pause, LV_OBJ_FLAG_HIDDEN); // Show "Paused" label
    }
    else {
        lv_obj_clear_flag(btn_reset, LV_OBJ_FLAG_HIDDEN); // Show Reset
        lv_obj_set_align(btn_start, LV_ALIGN_LEFT_MID);   // Align Start to left
        lv_obj_set_align(btn_reset, LV_ALIGN_RIGHT_MID);  // Align Reset to right
        lv_obj_add_flag(label_pause, LV_OBJ_FLAG_HIDDEN);
    }
}

#if 0
static void ui_update_state_text(PomodoroState_e state)
{
   switch (state) {
        case POMODORO_IDLE:
            lv_label_set_text(label_mode, "Ready");
            lv_label_set_text(lv_obj_get_child(btn_start, 0), "Start");
            lv_obj_add_flag(label_pause, LV_OBJ_FLAG_HIDDEN); // Hide "Paused" label
            update_timer_label(timer_get_remaining());
            break;
            
        case POMODORO_WORK:
            lv_label_set_text(label_mode, "Focus");
            lv_label_set_text(lv_obj_get_child(btn_start, 0), "Pause");
            break;
            
        case POMODORO_SHORT_BREAK:
            lv_label_set_text(label_mode, "Short Break");
            lv_label_set_text(lv_obj_get_child(btn_start, 0), "Pause");
            break;
            
        case POMODORO_LONG_BREAK:
            lv_label_set_text(label_mode, "Long Break");
            lv_label_set_text(lv_obj_get_child(btn_start, 0), "Pause");
            break;
            
        case POMODORO_PAUSED_WORK:
            lv_label_set_text(lv_obj_get_child(btn_start, 0), "Resume");
            // Keep current remaining time
            update_timer_label(timer_get_remaining());
            break;
            
        case POMODORO_PAUSED_BREAK:
            lv_label_set_text(lv_obj_get_child(btn_start, 0), "Resume");
            // Keep current remaining time
            update_timer_label(timer_get_remaining());
            break;
    }
}
#endif

static void ui_update_cycle_counter(void)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "Cycle: %d / %d", 
             pomodoro_get_current_cycle(),
             pomodoro_get_max_cycles());
    lv_label_set_text(label_cycle, buf);
    lv_obj_set_style_text_color(label_cycle, lv_color_hex(0x00FFFF), 0);
}

static void update_timer_and_progress(PomodoroState_e state)
{
    if (!pomodoro_is_resume_transition() && !pomodoro_is_pause_transition()) {
        lv_arc_set_range(progress, 0, pomodoro_get_remaining_sec());
    }

    // Update remaining time and progress bar
    update_timer_label(pomodoro_get_remaining_sec() * 1000);

    if (state == POMODORO_IDLE) {
        lv_obj_set_style_text_color(label_timer, lv_color_hex(0x4A90E2), 0);
        lv_obj_set_style_arc_color(progress, lv_color_hex(0x4A90E2), LV_PART_INDICATOR);
    }
}

static void pomodoro_state_changed(PomodoroState_e state)
{
    ui_main_screen_update_mode_icon(state);
    ui_update_ctrl_button(state);
    #if 0
    ui_update_state_text(state);
    #endif

    update_timer_and_progress(state);
    ui_update_cycle_counter();

    work_state_elapsed_sec = 0;
}

static void start_event_cb(lv_event_t *e)
{
    PomodoroState_e current_state = pomodoro_get_state();

    switch (current_state) {
        case POMODORO_IDLE:
            // Start new work session
            event_dispatch(EVENT_START, NULL);
            break;
            
        case POMODORO_WORK:
        case POMODORO_SHORT_BREAK:
        case POMODORO_LONG_BREAK:
            timer_pause();
            event_dispatch(EVENT_PAUSE, NULL);
            break;
            
        case POMODORO_PAUSED_WORK:
        case POMODORO_PAUSED_BREAK:
            timer_resume();
            event_dispatch(EVENT_RESUME, NULL);
            break;
    }
}

static void reset_event_cb(lv_event_t *e)
{
    event_dispatch(EVENT_RESET, NULL);
    timer_stop();
    pomodoro_state_changed(POMODORO_IDLE); // Force UI update
}

static void ui_tick_cb(uint32_t remaining) {
    update_timer_label(remaining);

    PomodoroState_e state = pomodoro_get_state();
    if (state == POMODORO_WORK) {
        if (fullscreen_enable) {
            work_state_elapsed_sec++;
            if (!fullscreen_timer_active && work_state_elapsed_sec >= POMO_MOVE_TO_FULLSCREEN_SEC) {
                // Show fullscreen overlay after 10 seconds
                show_fullscreen_timer(main_cont);
                fullscreen_timer_active = true;
            }
            if (fullscreen_timer_active) {
                update_fullscreen_timer(remaining);
            }
        }

        uint8_t percent = pomodoro_get_work_progress_in_percent();
        if (percent > 50 && percent < 80) {
            lv_obj_clear_flag(work_race_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_color(label_timer, lv_color_hex(0x9B59B6), 0);
            lv_obj_set_style_arc_color(progress, lv_color_hex(0x9B59B6), LV_PART_INDICATOR);
        }
        else if (percent >= 80) {
            lv_obj_clear_flag(work_speed_icon, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_color(label_timer, lv_color_hex(0xE74C3C), 0);
            lv_obj_set_style_arc_color(progress, lv_color_hex(0xE74C3C), LV_PART_INDICATOR);
        }
        else {
            lv_obj_set_style_text_color(label_timer, lv_color_hex(0x4A90E2), 0);
            lv_obj_set_style_arc_color(progress, lv_color_hex(0x4A90E2), LV_PART_INDICATOR);
        }
    }
    else if (state == POMODORO_SHORT_BREAK || state == POMODORO_LONG_BREAK) {
        lv_obj_set_style_text_color(label_timer, lv_color_hex(0x4A90E2), 0);
        lv_obj_set_style_arc_color(progress, lv_color_hex(0x4A90E2), LV_PART_INDICATOR);
    }
    else {
        // Not in WORK state: reset elapsed time and hide overlay if shown
        work_state_elapsed_sec = 0;
        if (fullscreen_timer_active) {
            hide_fullscreen_timer();
            fullscreen_timer_active = false;
        }
    }
}

static void timer_tick_cb(lv_timer_t * timer) {
    timer_tick_handler();
}

static void setting_event_cb(lv_event_t *e)
{
    LV_LOG_USER("Moving to Settings page...\n");
    show_settings_screen(main_cont);

}