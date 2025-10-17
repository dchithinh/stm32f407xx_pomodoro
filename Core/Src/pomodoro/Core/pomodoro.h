#ifndef __H_POMODORO_H__
#define __H_POMODORO_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POMODORO_DEF_WORK_MIN               1
#define POMODORO_DEF_SHORT_BREAK_MIN        1
#define POMODORO_DEF_LONG_BREAK_MIN         2
#define POMODORO_DEF_CYCLES_BEFORE_LONG     2

/**
 * @brief Pomodoro states
 */
typedef enum {
    POMODORO_IDLE,          /**< Timer not started */
    POMODORO_WORK,          /**< Work session running */
    POMODORO_SHORT_BREAK,   /**< Short break session running */
    POMODORO_LONG_BREAK,    /**< Long break session running */
    POMODORO_PAUSED_WORK,   /**< Work session paused */
    POMODORO_PAUSED_BREAK   /**< Break session paused */
} PomodoroState_e;

/**
 * @brief Type for state change callback
 * @param state New state
 */
typedef void (*pomodoro_state_cb_t)(PomodoroState_e state);

/**
 * @brief Type for tick callback (called every second)
 * @param remaining_sec Remaining seconds
 */
typedef void (*pomodoro_tick_cb_t)(uint32_t remaining_sec);

/**
 * @brief Initialize the Pomodoro module
 * @param work_min Duration of work session in minutes
 * @param short_break_min Duration of short break in minutes
 * @param long_break_min Duration of long break in minutes
 * @param cycles_before_long Number of work cycles before a long break
 */
void pomodoro_init(uint32_t work_min, uint32_t short_break_min,
                   uint32_t long_break_min, uint8_t cycles_before_long);

/**
 * @brief Start a Pomodoro timer
 *        Moves IDLE -> WORK
 */
void pomodoro_start(void);

/**
 * @brief Pause the current session
 *        WORK -> PAUSED_WORK
 *        BREAK -> PAUSED_BREAK
 */
void pomodoro_pause(void);

/**
 * @brief Resume a paused session
 *        PAUSED_WORK -> WORK
 *        PAUSED_BREAK -> SHORT/LONG_BREAK
 */
void pomodoro_resume(void);

/**
 * @brief Reset the Pomodoro timer
 *        Any state -> IDLE
 */
void pomodoro_reset(void);

/**
 * @brief Get the current state
 * @return Current PomodoroState_e
 */
PomodoroState_e pomodoro_get_state(void);

/**
 * @brief Get remaining seconds of current session
 * @return Remaining seconds
 */
uint32_t pomodoro_get_remaining_sec(void);

/**
 * @brief Get the current cycle count (number of completed work sessions)
 * @return Current cycle count
 */
uint8_t pomodoro_get_current_cycle(void);

/**
 * @brief Get the maximum number of cycles before a long break
 * @return Maximum cycles before a long break
 */
uint8_t pomodoro_get_max_cycles(void);

/**
 * @brief Register callback for state changes
 * @param cb Function pointer to call on state change
 */
void pomodoro_set_state_callback(pomodoro_state_cb_t cb);

/**
 * @brief Register callback for timer ticks
 * @param cb Function pointer to call every second
 */
void pomodoro_set_tick_callback(pomodoro_tick_cb_t cb);

/**
 * @brief To be called every second to update Pomodoro timer
 *        Normally this will be called from a timer interrupt or periodic task
 */
void pomodoro_tick(void);

/** 
 * @brief Check if the last state transition was a resume transition
 * @return true if the last transition was a resume, false otherwise
 */
bool pomodoro_is_resume_transition(void);

/** 
 * @brief Check if the current state is a paused state
 * @return true if the current state is PAUSED_WORK or PAUSED_BREAK, false otherwise
 */
bool pomodoro_is_pause_transition(void);

/**
 * @brief Update durations and cycle count
 * @param work_min New work duration in minutes
 * @param short_break_min New short break duration in minutes
 * @param long_break_min New long break duration in minutes
 * @param cycles_before_long New number of cycles before a long break
 */
void pomodoro_update_durations(uint32_t work_min, uint32_t short_break_min,
                               uint32_t long_break_min, uint8_t cycles_before_long);

/**
 * @brief Get the configured work time duration
 * @return Work time duration in minutes
 */
int pomodoro_get_work_time(void);

/**
 * @brief Get the configured short break duration
 * @return Short break duration in minutes
 */
int pomodoro_get_short_break(void);

/**
 * @brief Get the configured long break duration
 * @return Long break duration in minutes
 */
int pomodoro_get_long_break(void);

/**
 * @brief Get the configured number of cycles before long break
 * @return Number of cycles before long break
 */
int pomodoro_get_cycle_count(void);

/**
 * @brief Get the progress of current work session as a percentage
 * @details Computes how far along the current work session is by comparing 
 *          the remaining time against the total work duration
 * @return Percentage of work session completed (0-100)
 * @note Returns 0 if work duration is configured as 0
 */
uint8_t pomodoro_get_work_progress_in_percent(void);


int8_t pomodoro_get_pause_break_type(void);

#ifdef __cplusplus
}
#endif

#endif // __H_POMODORO_H__

