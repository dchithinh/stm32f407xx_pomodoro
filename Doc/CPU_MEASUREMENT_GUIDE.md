# CPU Usage Measurement Guide

## Why You're Not Seeing DMA Improvement

### Common Misconceptions

**Misconception:** "DMA should reduce total CPU usage"

**Reality:** DMA reduces CPU usage **during LCD transfers only**, not overall CPU load.

### What Actually Happens

#### CPU Time Breakdown (Without DMA)
```
Total 100ms period:
├─► LVGL rendering:        70ms  (70%)  ← Drawing widgets to buffer
├─► LCD flush (blocking):   25ms  (25%)  ← Waiting for SPI transfer
└─► Idle/other:             5ms   (5%)

CPU Usage: 95%
```

#### CPU Time Breakdown (With DMA)
```
Total 100ms period:
├─► LVGL rendering:        70ms  (70%)  ← Same (still need to draw)
├─► LCD flush (DMA setup):  1ms   (1%)  ← Just start DMA, return
├─► DMA transfer:          24ms  (async, CPU free!)
└─► Idle/other:            29ms  (29%)  ← CPU can do other work here

CPU Usage: 71%
```

**Improvement:** 95% → 71% = **24% reduction**

BUT: If you only measure "% CPU busy", you might see similar numbers because:
- LVGL rendering dominates (70%)
- DMA saves only 24% of the time

---

## How to Properly Measure DMA Performance

### Method 1: GPIO Toggle Timing

Add a GPIO toggle to measure **actual flush time**:

```c
// In main.h, add:
#define PERF_PIN_PORT   GPIOC
#define PERF_PIN        GPIO_PIN_13  // LED or any free pin

// Initialize in main():
GPIO_InitTypeDef gpio = {0};
gpio.Pin = PERF_PIN;
gpio.Mode = GPIO_MODE_OUTPUT_PP;
gpio.Speed = GPIO_SPEED_FREQ_HIGH;
HAL_GPIO_Init(PERF_PIN_PORT, &gpio);
```

```c
// In tft.c, modify tft_flush():
static void tft_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p)
{
    // ... existing code ...
    
    HAL_GPIO_WritePin(PERF_PIN_PORT, PERF_PIN, GPIO_PIN_SET);  // START marker
    
    lcd_write_pixels(color_p, total_bytes);
    
#if USE_DMA_FLUSH_LCD
    disp_drv_pending = disp;
#else
    HAL_GPIO_WritePin(PERF_PIN_PORT, PERF_PIN, GPIO_PIN_RESET);  // END marker (blocking)
    lv_disp_flush_ready(disp);
#endif
}

// In tft_dma_transfer_complete():
void tft_dma_transfer_complete(void)
{
    HAL_GPIO_WritePin(PERF_PIN_PORT, PERF_PIN, GPIO_PIN_RESET);  // END marker (DMA)
    if (disp_drv_pending != NULL) {
        lv_disp_flush_ready((lv_display_t *)disp_drv_pending);
        disp_drv_pending = NULL;
    }
}
```

**Measure with oscilloscope or logic analyzer:**
- **Blocking mode:** Pulse width = full flush time (~3.7ms)
- **DMA mode:** Pulse width = DMA complete time (~3.7ms), but CPU free during this!

### Method 2: Timestamp Logging

```c
// In tft.c:
static void tft_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p)
{
    static uint32_t flush_start_time = 0;
    
    // ... setup code ...
    
    flush_start_time = HAL_GetTick();
    lcd_write_pixels(color_p, total_bytes);
    
#if USE_DMA_FLUSH_LCD
    uint32_t dma_setup_time = HAL_GetTick() - flush_start_time;
    printf("DMA setup: %lu ms (CPU free after this)\n", dma_setup_time);
    disp_drv_pending = disp;
#else
    uint32_t blocking_time = HAL_GetTick() - flush_start_time;
    printf("Blocking flush: %lu ms (CPU blocked)\n", blocking_time);
    lv_disp_flush_ready(disp);
#endif
}

void tft_dma_transfer_complete(void)
{
    static uint32_t last_flush_start = 0;
    uint32_t total_time = HAL_GetTick() - last_flush_start;
    printf("DMA complete: %lu ms total\n", total_time);
    
    if (disp_drv_pending != NULL) {
        lv_disp_flush_ready((lv_display_t *)disp_drv_pending);
        disp_drv_pending = NULL;
    }
    last_flush_start = HAL_GetTick();
}
```

**Expected Output:**

**Blocking Mode:**
```
Blocking flush: 4 ms (CPU blocked)
Blocking flush: 3 ms (CPU blocked)
Blocking flush: 4 ms (CPU blocked)
```

**DMA Mode:**
```
DMA setup: 0 ms (CPU free after this)
DMA complete: 4 ms total
DMA setup: 0 ms (CPU free after this)
DMA complete: 3 ms total
```

### Method 3: Touch Responsiveness Test

**This is the BEST real-world test!**

```c
// In your Pomodoro app, add touch event logging:
void button_event_handler(lv_event_t *e)
{
    static uint32_t last_touch_time = 0;
    uint32_t now = HAL_GetTick();
    uint32_t response_time = now - last_touch_time;
    
    printf("Touch response: %lu ms\n", response_time);
    last_touch_time = now;
    
    // ... rest of handler ...
}
```

**Test procedure:**
1. Start timer so screen updates every second
2. Rapidly tap buttons during updates
3. Measure response time

**Expected results:**
- **Blocking mode:** Touch lag during screen updates (10-30ms delay)
- **DMA mode:** Consistent instant response (<5ms delay)

---

## Why DMA Might Not Show Improvement

### 1. You're Using `lcd_fill_rect()` for Testing

If you're calling:
```c
lcd_set_background_color(0xFF0000);  // This uses OLD buffer system!
```

This function still has busy-wait loops! Use LVGL instead:
```c
lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xFF0000), 0);
```

### 2. Small Partial Updates

LVGL uses **partial rendering**. If updating only small areas (e.g., timer label):
- **Blocking:** 100 bytes = 0.024ms (CPU blocked)
- **DMA:** 100 bytes = 0.024ms (DMA) + 0.1ms overhead = slower!

DMA overhead can **hurt** performance on tiny transfers.

**Solution:** Use larger LVGL buffers to batch updates:
```c
// In tft.c:
uint32_t buf_size = (20UL * 1024UL) / 2;  // Increase from 10KB to 20KB
```

### 3. Measuring Wrong Metric

**Don't measure:** "% CPU busy in while(1) loop"  
**Do measure:** "Time from flush start to flush complete"

### 4. UART Printf Overhead

If you have lots of debug printf():
```c
printf("Debug: ...\n");  // This can take 1-2ms per call!
```

**UART at 115200 baud:**
- 1 character = ~87μs
- 50 characters = ~4.3ms (same as LCD flush!)

Remove debug prints for accurate measurement.

---

## Definitive DMA Verification Test

Add this to your code:

```c
// Global counter
volatile uint32_t cpu_free_counter = 0;

// In main loop:
while(1) {
    cpu_free_counter++;  // Count how many times we loop
    HAL_Delay(5);
    lv_timer_handler();
}

// In SysTick or 1-second timer:
void print_cpu_stats(void)
{
    static uint32_t last_counter = 0;
    uint32_t current = cpu_free_counter;
    uint32_t loops_per_second = current - last_counter;
    
    printf("Main loop iterations/sec: %lu\n", loops_per_second);
    last_counter = current;
}
```

**Expected:**
- **Blocking mode:** ~150-200 iterations/sec (blocked during flush)
- **DMA mode:** ~250-350 iterations/sec (free during flush)

This shows CPU has more free time with DMA!

---

## Summary: Where to Look

### ✅ DMA IS Working If:
- `tft_dma_transfer_complete()` is being called
- UART shows "DMA transfer complete" messages
- Display updates correctly
- Touch works during display updates

### ❌ DMA NOT Helping If:
- Still using `lcd_fill_rect()` for testing
- Measuring total CPU % (LVGL rendering dominates)
- Buffer size too small (DMA overhead > savings)
- Too many UART debug prints

### 🎯 Best Way to Verify:
1. Use GPIO toggle + oscilloscope
2. Or measure touch responsiveness
3. Or count main loop iterations

---

Would you like me to add the GPIO toggle code or touch response measurement to your project?
