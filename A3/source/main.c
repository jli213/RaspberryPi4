/*
 * Adapted from: https://github.com/bztsrc/raspi3-tutorial/blob/master/07_delays/delays.c
 */

#include "gpio.h"
#include "uart.h"

// GPIO pin assignments
#define PIN_SW1     0      // Button A 
#define PIN_SW2     1     // Button B 
#define PIN_RED     4      // Red LED 
#define PIN_YELLOW  12     // Yellow LED
#define PIN_GREEN   16     // Green LED 

void printUART(char *str) {
    uart_puts(str);
}

/*
 * Set a GPIO pin HIGH.
 * Pins <32 use GPSET0, pins >=32 use GPSET1.
 */
static inline void gpio_set(int pin) {
    if (pin < 32) *GPSET0 = (1 << pin);
    else *GPSET1 = (1 << (pin - 32));
}
/*
 * Set a GPIO pin LOW.
 * Uses GPCLR registers similarly to gpio_set.
 */

static inline void gpio_clear(int pin) {
    if (pin < 32) *GPCLR0 = (1 << pin);
    else *GPCLR1 = (1 << (pin - 32));
}

/*
 * PWM for three outputs simultaneously.
 * Each LED/motor has its own duty cycle.
 *
 * All signals share the same time slices so they stay synchronized.
 */

//
void pwm_three(int r, int g, int y) {
    const int p = 2000;   // total period (microseconds)
    const int m = 100;    // resolution
    int d = p / m;        // time per slice

    for (int j = 0; j < m; j++) {

        // RED
        if (j < r) gpio_set(PIN_RED);
        else       gpio_clear(PIN_RED);

        // GREEN
        if (j < g) gpio_set(PIN_GREEN);
        else       gpio_clear(PIN_GREEN);

        // YELLOW
        if (j < y) gpio_set(PIN_YELLOW);
        else       gpio_clear(PIN_YELLOW);

        sleep_micro_seconds(d);
    }
}
/*
 * PDM state structure.
 * used for pulse density modulation.
 */

typedef struct {
    int context;
} pdm_state;


//Perform one step of Pulse Density Modulation (PDM). 
void pdm_step(int pin, pdm_state *s, int level_percent) {
    const int m = 100;

    s->context += level_percent;

    if (s->context >= m) {
        s->context -= m;
        gpio_set(pin);
    } else {
        gpio_clear(pin);
    }
}

/*
 * Read the logic level of a GPIO pin.
 * Returns 1 if HIGH, 0 if LOW.
 */
int read_pin(int pin) {
    if (pin < 32) return (*GPLEV0 & (1 << pin)) != 0;
    else return (*GPLEV1 & (1 << (pin - 32))) != 0;
}


/**
 * Get System Timer's counter
 */
unsigned long get_system_timer()
{
    unsigned int h=-1, l;

    // we must read MMIO area as two separate 32 bit reads
    h = *CHI;
    l = *CLO;

    // we have to repeat it if high word changed during read
    if(h != *CHI) {
        h = *CHI;
        l = *CLO;
    }

    // compose long value
    return ((unsigned long) h << 32) | l;
}

/**
 * Wait N microsec (with BCM System Timer)
 */
void sleep_micro_seconds(unsigned int delay)
{
    unsigned long start_time = get_system_timer();

    // Could theoretically do other things while waiting in this loop
    while (get_system_timer() - start_time < delay);
}

/*
 * Enable internal pull-up resistor on a GPIO input pin.
 * Prevents floating inputs for switches.
 */
void gpio_enable_pullup(int pin) {
    volatile unsigned int* reg = GPIO_PUP_PDN_CNTRL_REG0 + (pin / 16);
    int shift = (pin % 16) * 2;

    *reg = (*reg & ~(0b11 << shift)) | (0b01 << shift);
}



//init previous of SW1 and SW2 to 1 (not pressed)
int prev_sw1 = 1;
int prev_sw2 = 1;

/*
 * Update operating mode based on button presses.
 * Buttons are active-low because of pull-ups.
 * Mode changes on button press events (1->0 transitions).
*/
void update_mode(int *mode) {
    int sw1 = read_pin(PIN_SW1);
    int sw2 = read_pin(PIN_SW2);

    // Active-low buttons (pull-ups)
    if (prev_sw1 == 1 && sw1 == 0) {
        *mode = (*mode - 1 + 4) % 4; // + 4 to avoid negative values
    }
    if (prev_sw2 == 1 && sw2 == 0) {
        *mode = (*mode + 1) % 4;
    }

    prev_sw1 = sw1;
    prev_sw2 = sw2;
}

/*
 * Mode configuration structure
 */
typedef struct {
    int red;
    int green;
    int yellow;
    int use_pwm;   // 1 = PWM, 0 = PDM
} mode_t;

/*
 * Predefined operating modes for the boat controller.
 */
mode_t modes[4] = {
    {50, 50,  0, 1},   // Mode 0: forward (PWM)
    {10, 80,  0, 0},   // Mode 1: turn left (PDM)
    {80, 10,  0, 0},   // Mode 2: turn right (PDM)
    { 0,  0, 50, 1}    // Mode 3: reverse (PWM)
};



int main() {
    printUART("Boat controller starting...\n");

    // Configure LEDs as outputs
    *GPFSEL0 = (*GPFSEL0 & ~(0b111 << 12)) | (0b001 << 12);   // GPIO 4
    *GPFSEL1 = (*GPFSEL1 & ~(0b111 << 6))  | (0b001 << 6);    // GPIO 12
    *GPFSEL1 = (*GPFSEL1 & ~(0b111 << 18)) | (0b001 << 18);   // GPIO 16

    // Configure switches as inputs (GPIO 0,1)
    *GPFSEL0 &= ~(0b111 << 0);   // GPIO 0
    *GPFSEL0 &= ~(0b111 << 3);   // GPIO 1

    // Enable pull-ups for switches
    gpio_enable_pullup(PIN_SW1);
    gpio_enable_pullup(PIN_SW2);

    int mode = 0;

    // PDM states for each LED
    pdm_state red_pdm = {0};
    pdm_state green_pdm = {0};
    pdm_state yellow_pdm = {0};

    unsigned long last_button_check = get_system_timer();

    while (1) {

        // Check buttons every 20ms
        if (get_system_timer() - last_button_check > 20000) {
            update_mode(&mode);
            last_button_check = get_system_timer();
        }

        // Load parameters for current mode
        mode_t m = modes[mode];

        // Choose PWM or PDM based on mode configuration
        if (m.use_pwm) {
            pwm_three(m.red, m.green, m.yellow);
        } else {
            pdm_step(PIN_RED,    &red_pdm,    m.red);
            pdm_step(PIN_GREEN,  &green_pdm,  m.green);
            pdm_step(PIN_YELLOW, &yellow_pdm, m.yellow);
        }
    }

    return 0;
}


