#include "timer.h"
#include "lvgl.h"

#ifdef USE_STM32F407xx_HAL_TICK
    #include "stm32f4xx_hal.h"   // Or your MCU HAL header
#elif defined HAL_PICO
    //Nothing to include
#else
    #include <SDL.h>  // Add SDL include for tick counter
#endif


static struct Timer_t {
    bool running;
    bool paused;
    uint32_t duration;
    uint32_t start_tick;
    uint32_t pause_tick;
    uint32_t pause_duration;
    uint32_t last_tick;        // Add tracking of last tick time
    uint32_t current_remaining; // Add current remaining time
    void (*on_tick)(uint32_t);
    void (*on_finished)(void);
} tmr;


// Portable tick getter
static inline uint32_t get_tick_ms(void) {
    uint32_t tick = 0;
#ifdef USE_STM32F407xx_HAL_TICK
    tick = HAL_GetTick();
#elif defined HAL_PICO
    extern uint32_t tick_timer();
    tick = tick_timer();
#else
    tick = SDL_GetTicks();  // Use SDL's tick counter instead of fake increment
#endif

    return tick;
}

void timer_init(void) {
    tmr.duration = 0;
    tmr.start_tick = 0;
    tmr.on_tick = 0;
    tmr.on_finished = 0;
    tmr.running = false;
    tmr.paused = false;
    tmr.pause_duration = 0;
    tmr.current_remaining = 0;
}

void timer_start(uint32_t ms,
                 void (*on_tick)(uint32_t),
                 void (*on_finished)(void)) {
    tmr.duration = ms;
    tmr.start_tick = get_tick_ms();
    tmr.on_tick = on_tick;
    tmr.on_finished = on_finished;
    tmr.running = true;
    tmr.paused = false;

    tmr.current_remaining = ms; // Initialize current remaining time
    tmr.pause_duration = 0;
}

void timer_stop(void) {
    tmr.running = false;
    tmr.paused = false;
}

void timer_restart(uint32_t ms) {
    if (tmr.on_tick || tmr.on_finished) {
        timer_start(ms, tmr.on_tick, tmr.on_finished);
    }
}

bool timer_is_running(void) {
    return tmr.running;
}

void timer_tick_handler(void)
{
    if (!tmr.running || tmr.paused) {
        // LV_LOG_USER("[TIMER] Not running or paused\n");
        return;
    }

    uint32_t now = get_tick_ms();
    uint32_t elapsed = now - tmr.start_tick - tmr.pause_duration;
    
    // Store current remaining time
    tmr.current_remaining = (elapsed >= tmr.duration) ? 0 : (tmr.duration - elapsed);
    tmr.last_tick = now;
    
    if (tmr.current_remaining > 0) {
        if (tmr.on_tick) {
            tmr.on_tick(tmr.current_remaining);
        }
    } else {
        tmr.running = false;
        if (tmr.on_finished) {
            tmr.on_finished();
        }
    }

}

void timer_pause(void)
{
    if (tmr.running && !tmr.paused) {
        tmr.paused = true;
        tmr.pause_tick = tmr.last_tick; // Use last tick time instead of current time
        
        // Use stored remaining time from last tick
        if (tmr.on_tick) {
            tmr.on_tick(tmr.current_remaining);
        }
    }
}

void timer_resume(void) {
    if (tmr.paused) {
        // Calculate how long we were paused
        uint32_t now = get_tick_ms();
        tmr.pause_duration += (now - tmr.pause_tick);
        
        // Clear pause state
        tmr.paused = false;
    }
}

uint32_t timer_get_remaining(void)
{
    if (!tmr.running) return tmr.duration;
    if (tmr.paused) return tmr.current_remaining;
    
    uint32_t now = get_tick_ms();
    uint32_t elapsed = now - tmr.start_tick - tmr.pause_duration;
    return (elapsed >= tmr.duration) ? 0 : (tmr.duration - elapsed);
}
