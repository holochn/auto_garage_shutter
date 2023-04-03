#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/rosc.h"
#include "hardware/structs/scb.h"
#include "pico/sleep.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>

static const uint PIN_DOOR_CTRL = 9;
static const uint PIN_DOOR_STAT = 26;
static const uint PIN_PWR_CTRL = 16;

static const uint8_t SLEEP_MIN = 10;
static const uint8_t SLEEP_SEC = 0;

static const uint16_t DOOR_OPEN_THRSHLD = 5;
static const float ADC_CONV_FACT = 3.3f / (1 << 12);

static void gotoSleep(uint8_t min, uint8_t sec);
static void recover_from_sleep(uint scb_orig, uint clock0_orig,
                               uint clock1_orig);
static void rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to,
                      void (*callback)(void));
static void sleep_callback(void) {}
static void shutDown(void);
static bool doorOpen(uint16_t threshold);
static void closeDoor(void);

int main() {

  float current_door_state = 0.0f;

  stdio_init_all();

  // save values for later
  uint scb_orig = scb_hw->scr;
  uint clock0_orig = clocks_hw->sleep_en0;
  uint clock1_orig = clocks_hw->sleep_en1;

  if (doorOpen(DOOR_OPEN_THRSHLD)) {
    gotoSleep(SLEEP_MIN, SLEEP_SEC);
    // reset processor and clocks back to defaults
    recover_from_sleep(scb_orig, clock0_orig, clock1_orig);
    if (doorOpen(DOOR_OPEN_THRSHLD)) {
      closeDoor();
    }
  }

  shutDown();

  // we only see blinking if something really went wrong
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  while (true) {
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(500);
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(500);
  }

  return 0;
}

/* @brief: Prepares sleeping by switching to XOSC
 * @param: min = Minutes to sleep
 *         sec = Seconds to sleep
 */
void gotoSleep(uint8_t min, uint8_t sec) {
  // UART will be reconfigured by sleep_run_from_xosc
  sleep_run_from_xosc();

  // At this point the pico will sleep
  rtc_sleep(min, sec, &sleep_callback);
}

/* @brief: Recovers all settings after sleeping
 * @param: scb_orig = value of the scb register
 *          clock0_orig = original value of the sleep enable 0 register
 *          clock1_orig = original value of the sleep enable 1 register
 */
void recover_from_sleep(uint scb_orig, uint clock0_orig, uint clock1_orig) {

  // Re-enable ring Oscillator control
  rosc_write(&rosc_hw->ctrl, ROSC_CTRL_ENABLE_BITS);

  // reset procs back to default
  scb_hw->scr = scb_orig;
  clocks_hw->sleep_en0 = clock0_orig;
  clocks_hw->sleep_en1 = clock1_orig;

  // reset clocks
  clocks_init();
  stdio_init_all();

  return;
}

/* @brief: sets the RTC and sends pico to sleep
 * @param: minute_to_sleep_to = minutes to sleep
 *          second_to_sleep_to = seonds to sleep
 *          callback = function pointer to wake up callback
 *
 */
static void rtc_sleep(int8_t minute_to_sleep_to, int8_t second_to_sleep_to,
                      void (*callback)(void)) {
  // Start on Friday 5th of June 2020 15:45:00
  datetime_t t = {.year = 2020,
                  .month = 06,
                  .day = 05,
                  .dotw = 5, // 0 is Sunday, so 5 is Friday
                  .hour = 00,
                  .min = 00,
                  .sec = 00};

  // Alarm 10 minutes later
  datetime_t t_alarm = {.year = 2020,
                        .month = 06,
                        .day = 05,
                        .dotw = 5, // 0 is Sunday, so 5 is Friday
                        .hour = 00,
                        .min = minute_to_sleep_to,
                        .sec = second_to_sleep_to};

  // Start the RTC
  rtc_init();
  rtc_set_datetime(&t);
  sleep_goto_sleep_until(&t_alarm, callback);
}

/* @brief: Shuts down the board by triggering a transistor connected on GPIO
 *
 */
static void shutDown(void) {
  gpio_init(PIN_PWR_CTRL);
  gpio_set_dir(PIN_PWR_CTRL, GPIO_OUT);

  gpio_put(PIN_PWR_CTRL, true);
}

/* @brief: closes door by triggering a GPIO
 *
 */
static void closeDoor(void) {
  gpio_init(PIN_DOOR_CTRL);
  gpio_set_dir(PIN_DOOR_CTRL, GPIO_OUT);

  gpio_put(PIN_DOOR_CTRL, true);
}

/* @brief: checks if ADC value indicates door open state or not
 * @param: none
 * @return: true if door is open
 *          false if closed
 */
static bool doorOpen(uint16_t treshold) {
  uint16_t adc_raw = 0;

  // init ADC for door state
  adc_init();
  adc_gpio_init(PIN_DOOR_STAT);
  adc_select_input(0);

  adc_raw = adc_read();
  // uncomment, in case we botheer about the real value
  // current_door_state = adc_raw * ADC_CONV_FACT;

  if (adc_raw >= treshold) {
    return true;
  }
  return false;
}
