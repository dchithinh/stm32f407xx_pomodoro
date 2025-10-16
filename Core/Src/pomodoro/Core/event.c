#include "event.h"
#include "pomodoro.h"

/**
 * @file event.c
 * @brief Event dispatcher implementation.
 *
 * This file translates UI events into Pomodoro core API calls.
 */

// ================== Public API ==================

#define POMODORO_DEF_WORK_MIN               1
#define POMODORO_DEF_SHORT_BREAK_MIN        1
#define POMODORO_DEF_LONG_BREAK_MIN         2
#define POMODORO_DEF_CYCLES_BEFORE_LONG     2

void event_init(void) {
    pomodoro_init(pomodoro_get_work_time() / (60 * 1000),
                  pomodoro_get_short_break() /  (60 * 1000),
                  pomodoro_get_long_break() /  (60 * 1000),
                  pomodoro_get_cycle_count());
}

void event_dispatch(EventType_e type, void *data) {
    switch (type) {
    case EVENT_START:
        pomodoro_start();
        break;

    case EVENT_PAUSE:
        pomodoro_pause();
        break;

    case EVENT_RESUME:
        pomodoro_resume();
        break;

    case EVENT_RESET:
        pomodoro_reset();
        break;

    case EVENT_SETTINGS: {
        PomodoroSettings_t *s = (PomodoroSettings_t *)data;
        if (s) {
            pomodoro_init(s->work_min,
                          s->short_break_min,
                          s->long_break_min,
                          s->cycles_before_long);
        }
        break;
    }

    case EVENT_TICK:
        // Optional: could call pomodoro_tick() if using manual tick
        break;

    default:
        // Unknown event -> ignore
        break;
    }
}
