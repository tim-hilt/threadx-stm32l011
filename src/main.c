// dep
#include "tx_api.h"

// local
#include "stm32l0xx.h"

void init_leds();

void main(void) {
  init_leds();

  tx_kernel_enter();
}

void init_leds() {
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
  GPIOB->MODER ^= GPIO_MODER_MODE3_1;
}

TX_THREAD toggle_on_thread;
TX_THREAD toggle_off_thread;

void toggle_on(uint32_t);
void toggle_off(uint32_t);

void tx_application_define(void* first_unused_memory) {
  static char thread_name_toggle_on[] = "toggle_on";
  static char toggle_on_stack[128];
  tx_thread_create(&toggle_on_thread,
      thread_name_toggle_on,
      toggle_on,
      0x00000000,
      toggle_on_stack,
      sizeof(toggle_on_stack),
      3,
      3,
      TX_NO_TIME_SLICE,
      TX_AUTO_START);
  static char thread_name_toggle_off[] = "toggle_off";
  static char toggle_off_stack[128];
  tx_thread_create(&toggle_off_thread,
      thread_name_toggle_off,
      toggle_off,
      0x00000000,
      toggle_off_stack,
      sizeof(toggle_off_stack),
      3,
      3,
      TX_NO_TIME_SLICE,
      TX_AUTO_START);
}

void toggle_on(uint32_t thread_input) {
  while (1) {
    GPIOB->BSRR = GPIO_BSRR_BS_3;
    tx_thread_sleep(20);
  }
}

void toggle_off(uint32_t thread_input) {
  while (1) {
    tx_thread_sleep(10);
    GPIOB->BSRR = GPIO_BSRR_BR_3;
    tx_thread_sleep(10);
  }
}