## Main UI Screen

```
 ---------------------------------
|          POMODORO APP           |
|---------------------------------|
|          [ Work Mode ]          | <- State label
|                                 |
|           ( 25:00 )             | <- Timer label
|       ⭕ Progress Arc ⭕       |
|                                 |
| [ Start ] [ Pause ] [ Reset ]   | <- Buttons
|                                 |
|         Cycle: 1 / 4            | <- Status
 ---------------------------------

```

## High level Structure
```
Pomodoro App
│
├─ UI   <- Responsible for rendering and interaction
│   ├─ main_screen.c/h      <- Main Pomodoro UI: timer label, progress arc, buttons, status label
│   ├─ settings_screen.c/h  <- Optional: change work/break duration, cycles, theme
│   └─ ui_helpers.c/h       <- Utility functions: create buttons, labels, arcs, common styles
│
├─ Core     <- Handles timer and state machine
│   ├─ pomodoro.c/h    <- State machine: WORK / SHORT_BREAK / LONG_BREAK
│   ├─ timer.c/h       <- Countdown logic, tick callback
│   └─ event.c/h       <- Events from UI: start/pause/reset, state changes
│
└─ main.c             <- Initialize LVGL, hardware, call UI and Core logic

```

## Architecture Overview
```
┌─────────────────────────────────────────────────────────────────┐
│                        Application Layer                        │
│                          (main_screen.c)                        │
└─────────────────────────┬───────────────────────────────────────┘
                          │ UI Callbacks
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Pomodoro State Machine                       │
│                        (pomodoro.c)                             │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ States: IDLE → WORK → SHORT_BREAK → LONG_BREAK              │ │
│ │ Logic: Cycles, Transitions, Pause/Resume                    │ │
│ │ Callbacks: on_timer_tick(), on_timer_finished()             │ │
│ └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────┬───────────────────────────────────────┘
                          │ Timer Control
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Timer Abstraction                           │
│                        (timer.c)                                │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ Hardware: start/stop/pause/resume/tick_handler              │ │
│ │ Platform: STM32/Pico/SDL tick sources                       │ │
│ │ Timing: Accurate pause/resume with duration tracking        │ │
│ └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────┬───────────────────────────────────────┘
                          │ Platform API
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Hardware Abstraction                         │
│            HAL_GetTick() / SDL_GetTicks() / etc.                │
└─────────────────────────────────────────────────────────────────┘
```

## State Transition Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    POMODORO STATE MACHINE                       │
└─────────────────────────────────────────────────────────────────┘
                          │
                          ▼
          pomodoro_start() calls timer_start()
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      TIMER RUNNING                              │
│ Every 1000ms: timer_tick_handler()                              │
│               └─► on_timer_tick(remaining_ms)                   │
│                   └─► pomodoro.c updates remaining_ms           │
│                       └─► UI callback with remaining time       │
└─────────────────────────┬───────────────────────────────────────┘
                          │ When timer expires
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TIMER FINISHED                               │
│ on_timer_finished() called                                      │
│ └─► pomodoro.c handles state transition                         │
│     ├─► WORK → SHORT_BREAK/LONG_BREAK                           │
│     └─► BREAK → WORK                                            │
│         └─► timer_start() with new duration                     │
└─────────────────────────────────────────────────────────────────┘
```