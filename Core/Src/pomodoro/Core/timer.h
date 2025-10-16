#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include <stdbool.h>

// #define USE_HAL_TICK   1

/// Initialize timer system
void timer_init(void);

/// Start a countdown timer
/// @param ms duration in milliseconds
/// @param on_tick callback for each tick (can be NULL). Parameter = remaining time in ms
/// @param on_finished callback when timer expires (can be NULL)
void timer_start(uint32_t ms,
                 void (*on_tick)(uint32_t),
                 void (*on_finished)(void));

/// Stop the current timer
void timer_stop(void);

/// Restart timer with new duration (keeps same callbacks)
void timer_restart(uint32_t ms);

/// Check if timer is running
bool timer_is_running(void);

/// Get remaining time (ms), 0 if stopped
uint32_t timer_get_remaining(void);

/// Pause and resume
void timer_pause(void);
void timer_resume(void);

/// To be called periodically from main loop or SysTick
void timer_tick_handler(void);

#endif // TIMER_H
