#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

// WS2812 Configuration
#define WS2812_PIN 25
#define NUM_PIXELS 1
#define FADE_STEPS 100
#define FADE_DURATION_MS 2500
#define STEP_DELAY_MS (FADE_DURATION_MS / FADE_STEPS)

// GPIO Toggling Configuration
#define GPIO_TOGGLE_INTERVAL_MS 2000  // 2 seconds between GPIO toggles
const uint GPIO_PINS[] = {0,1,2,3,4,5,6,7,8,9,10,11,22,26,27,28};
const uint NUM_GPIO_PINS = sizeof(GPIO_PINS)/sizeof(GPIO_PINS[0]);

// Initialize WS2812
void ws2812_init(PIO pio, uint sm, uint offset, uint pin, float freq, bool rgbw) {
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_shift(&c, false, true, rgbw ? 32 : 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    int cycles_per_bit = ws2812_T1 + ws2812_T2 + ws2812_T3;
    float div = clock_get_hz(clk_sys) / (freq * cycles_per_bit);
    sm_config_set_clkdiv(&c, div);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// Set WS2812 pixel color
void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Convert RGB to GRB format
uint32_t rgb_to_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

// Color fading effect
void fade_color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i <= FADE_STEPS; i++) {
        uint8_t current_r = (r * i) / FADE_STEPS;
        uint8_t current_g = (g * i) / FADE_STEPS;
        uint8_t current_b = (b * i) / FADE_STEPS;
        put_pixel(rgb_to_grb(current_r, current_g, current_b));
        sleep_ms(STEP_DELAY_MS);
    }
}

// Initialize all GPIO pins
void init_gpio_pins() {
    for (uint i = 0; i < NUM_GPIO_PINS; i++) {
        gpio_init(GPIO_PINS[i]);
        gpio_set_dir(GPIO_PINS[i], GPIO_OUT);
        gpio_put(GPIO_PINS[i], 0);  // Start with all pins low
    }
}

// Toggle all GPIO pins simultaneously
void toggle_gpio_pins() {
    for (uint i = 0; i < NUM_GPIO_PINS; i++) {
        gpio_put(GPIO_PINS[i], !gpio_get(GPIO_PINS[i]));
    }
}

int main() {
    stdio_init_all();
    
    // Initialize WS2812
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_init(pio, 0, offset, WS2812_PIN, 800000, false);
    
    // Initialize GPIO pins
    init_gpio_pins();
    
    // Timestamp for GPIO toggling
    absolute_time_t next_toggle_time = get_absolute_time();
    
    while (1) {
        // Run WS2812 color sequence
        fade_color(255, 0, 0);    // Red
        fade_color(0, 255, 0);    // Green
        fade_color(0, 0, 255);    // Blue
        fade_color(255, 255, 255);// White
        put_pixel(rgb_to_grb(0, 0, 0));//black
        // GPIO toggling phase
        while (true) {
            absolute_time_t current_time = get_absolute_time();
            if (absolute_time_diff_us(next_toggle_time, current_time) >= 0) {
                toggle_gpio_pins();
                next_toggle_time = delayed_by_ms(current_time, GPIO_TOGGLE_INTERVAL_MS);
            }
            
            // Small delay to prevent busy waiting
            sleep_ms(10);
        }
    }
    
    return 0;
}