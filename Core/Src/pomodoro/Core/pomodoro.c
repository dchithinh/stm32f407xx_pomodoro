#include <stdio.h>
#include "lvgl.h"
#include "pomodoro.h"
#include "timer.h"

// ====================== Data Structures ======================

/**
 * @brief Pomodoro session configuration
 */
typedef struct {
    uint32_t    work_duration_ms;          /**< Work session duration in milliseconds */
    uint32_t    short_break_duration_ms;   /**< Short break duration in milliseconds */
    uint32_t    long_break_duration_ms;    /**< Long break duration in milliseconds */
    uint8_t     max_cycles;                /**< Cycles before long break */
} PomodoroConfig_t;

/**
 * @brief Pomodoro session state
 */
typedef struct {
    PomodoroState_e current_state;      /**< Current session state */
    PomodoroState_e previous_state;     /**< Previous session state */
    uint32_t        remaining_ms;       /**< Remaining milliseconds in current session */
    uint8_t         cycle_count;        /**< Work sessions completed */
} PomodoroSession_t;

/**
 * @brief Pomodoro callback functions
 */
typedef struct {
    pomodoro_state_cb_t state_callback; /**< State change callback */
    pomodoro_tick_cb_t  tick_callback;  /**< Timer tick callback */
} PomodoroCallbacks_t;

/**
 * @brief Main Pomodoro context
 */
typedef struct {
    PomodoroConfig_t config;            /**< Session configuration */
    PomodoroSession_t session;          /**< Current session state */
    PomodoroCallbacks_t callbacks;      /**< Registered callbacks */
} PomodoroContext_t;

// ====================== Internal State ======================
static PomodoroContext_t pomo_ctx = {
    .config = {
        .work_duration_ms = POMODORO_DEF_WORK_MIN * 60 * 1000,
        .short_break_duration_ms = POMODORO_DEF_SHORT_BREAK_MIN * 60 * 1000,
        .long_break_duration_ms = POMODORO_DEF_LONG_BREAK_MIN * 60 * 1000,
        .max_cycles = POMODORO_DEF_CYCLES_BEFORE_LONG
    },
    .session = {
        .current_state = POMODORO_IDLE,
        .previous_state = POMODORO_IDLE,
        .remaining_ms = POMODORO_DEF_WORK_MIN * 60 * 1000,
        .cycle_count = 0
    },
    .callbacks = {
        .state_callback = NULL,
        .tick_callback = NULL
    }
};

static const char *pomoState2Str(PomodoroState_e state);
// ====================== Private Functions ======================
/**
 * @brief Change state internally and trigger callback
 * @param new_state New Pomodoro state
 * @param duration_ms Duration for the new state
 */
static void change_state(PomodoroState_e new_state, uint32_t duration_ms) {
    pomo_ctx.session.previous_state = pomo_ctx.session.current_state;
    pomo_ctx.session.current_state = new_state;
    pomo_ctx.session.remaining_ms = duration_ms;

    if (pomo_ctx.callbacks.state_callback) {
        pomo_ctx.callbacks.state_callback(pomo_ctx.session.current_state); //UI callback to update display
    }
}

// Timer tick callback
static void on_timer_tick(uint32_t remaining_ms) 
{
    pomo_ctx.session.remaining_ms = remaining_ms;  // Store current remaining time
    if (pomo_ctx.callbacks.tick_callback) {
        pomo_ctx.callbacks.tick_callback(pomo_ctx.session.remaining_ms);  // Pass current remaining time to UI
    }
}

// Timer finished callback
static void on_timer_finished(void) {
    LV_LOG_USER("[Pomodoro] Timer finished in state %s\n", pomoState2Str(pomo_ctx.session.current_state));
    if (pomo_ctx.session.current_state == POMODORO_WORK) {
        pomo_ctx.session.cycle_count++;
        if (pomo_ctx.session.cycle_count % pomo_ctx.config.max_cycles == 0) {
            change_state(POMODORO_LONG_BREAK, pomo_ctx.config.long_break_duration_ms);
            timer_start(pomo_ctx.config.long_break_duration_ms, on_timer_tick, on_timer_finished);
        } else {
            change_state(POMODORO_SHORT_BREAK, pomo_ctx.config.short_break_duration_ms);
            timer_start(pomo_ctx.config.short_break_duration_ms, on_timer_tick, on_timer_finished);
        }
    } else { // Break finished
        change_state(POMODORO_WORK, pomo_ctx.config.work_duration_ms);
        timer_start(pomo_ctx.config.work_duration_ms, on_timer_tick, on_timer_finished);
    }
}

void pomodoro_init(uint32_t work_min, uint32_t short_break_min,
                   uint32_t long_break_min, uint8_t cycles_before_long) {
    pomo_ctx.config.work_duration_ms = work_min * 60 * 1000;
    pomo_ctx.config.short_break_duration_ms = short_break_min * 60 * 1000;
    pomo_ctx.config.long_break_duration_ms = long_break_min * 60 * 1000;
    pomo_ctx.config.max_cycles = cycles_before_long;
    pomo_ctx.session.cycle_count = 0;
    pomo_ctx.session.current_state = POMODORO_IDLE;
    pomo_ctx.session.remaining_ms = pomo_ctx.config.work_duration_ms;
}

void pomodoro_start(void) {
    if (pomo_ctx.session.current_state == POMODORO_IDLE) {
        change_state(POMODORO_WORK, pomo_ctx.config.work_duration_ms);
        timer_start(pomo_ctx.config.work_duration_ms, on_timer_tick, on_timer_finished);
    }
}

void pomodoro_pause(void)
{
    switch (pomo_ctx.session.current_state) {
        case POMODORO_WORK:
            change_state(POMODORO_PAUSED_WORK, timer_get_remaining());
            timer_pause();
            break;
            
        case POMODORO_SHORT_BREAK:
        case POMODORO_LONG_BREAK:
            change_state(POMODORO_PAUSED_BREAK, timer_get_remaining());
            timer_pause();
            break;
    }
}

void pomodoro_resume(void) {
    if (pomo_ctx.session.current_state == POMODORO_PAUSED_WORK) {
        change_state(POMODORO_WORK, pomo_ctx.session.remaining_ms);
        timer_resume();
    } else if (pomo_ctx.session.current_state == POMODORO_PAUSED_BREAK) {
        if (pomo_ctx.session.cycle_count % pomo_ctx.config.max_cycles == 0) {
            change_state(POMODORO_LONG_BREAK, pomo_ctx.session.remaining_ms);
        } else {
            change_state(POMODORO_SHORT_BREAK, pomo_ctx.session.remaining_ms);
        }
        timer_resume();
    }
}

void pomodoro_reset(void) {
    change_state(POMODORO_IDLE, 0);
    pomo_ctx.session.cycle_count = 0;
    pomo_ctx.session.remaining_ms = pomo_ctx.config.work_duration_ms;
    timer_stop();
}

PomodoroState_e pomodoro_get_state(void) {
    return pomo_ctx.session.current_state;
}

uint32_t pomodoro_get_remaining_sec(void) 
{
    return pomo_ctx.session.remaining_ms / 1000;  // Convert to seconds for API
}

void pomodoro_set_state_callback(pomodoro_state_cb_t cb)
{
    pomo_ctx.callbacks.state_callback = cb;
}

void pomodoro_set_tick_callback(pomodoro_tick_cb_t cb)
{
    pomo_ctx.callbacks.tick_callback = cb;
}

uint8_t pomodoro_get_current_cycle(void)
{
    return pomo_ctx.session.cycle_count;
}

uint8_t pomodoro_get_max_cycles(void)
{
    return pomo_ctx.config.max_cycles;
}

bool pomodoro_is_resume_transition(void)
{
    // True if previous_state was PAUSED_WORK and current_state is WORK,
    // or previous_state was PAUSED_BREAK and current_state is SHORT_BREAK or LONG_BREAK
    return ((pomo_ctx.session.previous_state == POMODORO_PAUSED_WORK && pomo_ctx.session.current_state == POMODORO_WORK) ||
            (pomo_ctx.session.previous_state == POMODORO_PAUSED_BREAK && 
               (pomo_ctx.session.current_state == POMODORO_SHORT_BREAK || pomo_ctx.session.current_state == POMODORO_LONG_BREAK)));
}

bool pomodoro_is_pause_transition(void)
{
    return (
        (pomo_ctx.session.current_state == POMODORO_PAUSED_WORK ||
         pomo_ctx.session.current_state == POMODORO_PAUSED_BREAK)
    );
}

int8_t pomodoro_get_pause_break_type(void)
{
    if (pomo_ctx.session.current_state == POMODORO_PAUSED_BREAK) {
        // Check the previous state to know which break was paused
        if (pomo_ctx.session.previous_state == POMODORO_SHORT_BREAK) {
            return POMODORO_SHORT_BREAK;
        }
        if (pomo_ctx.session.previous_state == POMODORO_LONG_BREAK) {
            return POMODORO_LONG_BREAK;
        }
    }

    return 0;
}

void pomodoro_update_durations(uint32_t work_min, uint32_t short_break_min,
                               uint32_t long_break_min, uint8_t cycles_before_long)
{
    pomo_ctx.config.work_duration_ms = work_min * 60 * 1000;
    pomo_ctx.config.short_break_duration_ms = short_break_min * 60 * 1000;
    pomo_ctx.config.long_break_duration_ms = long_break_min * 60 * 1000;
    pomo_ctx.config.max_cycles = cycles_before_long;

    // If currently idle, update remaining_ms to new work duration
    if (pomo_ctx.session.current_state == POMODORO_IDLE) {
        pomo_ctx.session.remaining_ms = pomo_ctx.config.work_duration_ms;
    }
}

int pomodoro_get_work_time(void)
{
    return pomo_ctx.config.work_duration_ms;
}

int pomodoro_get_short_break(void)
{
    return pomo_ctx.config.short_break_duration_ms;
}

int pomodoro_get_long_break(void)
{
    return pomo_ctx.config.long_break_duration_ms;
}

int pomodoro_get_cycle_count(void)
{
   return pomo_ctx.config.max_cycles;
}

uint8_t pomodoro_get_work_progress_in_percent(void)
{
    uint8_t percent = 0;
    if (pomo_ctx.config.work_duration_ms == 0) return 0;

    percent = (uint8_t)(((pomo_ctx.config.work_duration_ms - pomo_ctx.session.remaining_ms) * 100) /
                            pomo_ctx.config.work_duration_ms);
    return percent;
}

static const char *pomoState2Str(PomodoroState_e state) {
    switch (state) {
        case POMODORO_IDLE: return "IDLE";
        case POMODORO_WORK: return "WORK";
        case POMODORO_SHORT_BREAK: return "SHORT_BREAK";
        case POMODORO_LONG_BREAK: return "LONG_BREAK";
        case POMODORO_PAUSED_WORK: return "PAUSED_WORK";
        case POMODORO_PAUSED_BREAK: return "PAUSED_BREAK";
        default: return "UNKNOWN";
    }
}