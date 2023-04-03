#include <stdint.h>
#include <stdio.h>
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"

static const uint PIN_DOOR_CTRL = 9;
static const uint PIN_DOOR_STAT = 26;
static const uint PIN_PWR_CTRL  = 16;

static const uint8_t SLEEP_MIN = 10;
static const uint8_t SLEEP_SEC = 0;

static const uint16_t DOOR_OPEN_THRSHLD = 5;
static const float ADC_CONV_FACT = 3.3f / (1 << 12);

static void gotoSleep(uint8_t min, uint8_t sec);
static void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig);
static void rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to, void (*callback)(void));
static void sleep_callback(void) {}

static const int LED_PIN = PICO_DEFAULT_LED_PIN;

int main() {
    uint16_t adc_raw = 0;
    float current_door_state = 0.0f;

    stdio_init_all();

    // init ADC for door state
    adc_init();
    adc_gpio_init(PIN_DOOR_STAT);
    adc_select_input(0);

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //save values for later
    uint scb_orig = scb_hw->scr;
    uint clock0_orig = clocks_hw->sleep_en0;
    uint clock1_orig = clocks_hw->sleep_en1;

    adc_raw = adc_read();
    current_door_state = adc_raw * ADC_CONV_FACT;
    
    // if( adc_raw <= DOOR_OPEN_THRSHLD ) {
        printf("Go to sleep\n");
        gpio_put(LED_PIN, 1);
        // gotoSleep();

        //reset processor and clocks back to defaults
        // recover_from_sleep(scb_orig, clock0_orig, clock1_orig); 
    // }

    while(true) {
        gpio_put(LED_PIN, 0);
        sleep_ms(1000);
        printf("Hello, ");
        gpio_put(LED_PIN, 1);
        sleep_ms(1000);
        printf("world!\n");
        printf("ADC raw: %d\n", adc_raw);
    }

    return 0;
}

void gotoSleep(uint8_t min, uint8_t sec) {
    // UART will be reconfigured by sleep_run_from_xosc
    sleep_run_from_xosc();
    
    //At this point the pico will sleep
    rtc_sleep(min, sec, &sleep_callback);
}

void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig){

    //Re-enable ring Oscillator control
    rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

    //reset procs back to default
    scb_hw->scr = scb_orig;
    clocks_hw->sleep_en0 = clock0_orig;
    clocks_hw->sleep_en1 = clock1_orig;

    //reset clocks
    clocks_init();
    stdio_init_all();

    return;
}

static void rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to, void (*callback)(void)) {
    // Start on Friday 5th of June 2020 15:45:00
    datetime_t t = {
            .year  = 2020,
            .month = 06,
            .day   = 05,
            .dotw  = 5, // 0 is Sunday, so 5 is Friday
            .hour  = 00,
            .min   = 00,
            .sec   = 00
    };

    // Alarm 10 minutes later
    datetime_t t_alarm = {
            .year  = 2020,
            .month = 06,
            .day   = 05,
            .dotw  = 5, // 0 is Sunday, so 5 is Friday
            .hour  = 00,
            .min   = minute_to_sleep_to,
            .sec   = second_to_sleep_to
    };

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);
    sleep_goto_sleep_until(&t_alarm, callback);
}
