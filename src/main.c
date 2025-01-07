// dep
#include "tx_api.h"
// local
#include "stm32l0xx.h"

TX_THREAD toggle_on_thread;
TX_THREAD toggle_off_thread;

void init_leds(void);
void toggle_on(uint32_t);
void toggle_off(uint32_t);
void increase_clock_speed(void);

int main(void) {
  increase_clock_speed();
  init_leds();
  tx_kernel_enter();
}

// TODO: Try to understand every step that's done here
void increase_clock_speed(void) {
  FLASH->ACR |= FLASH_ACR_LATENCY;
  while ((FLASH->ACR & FLASH_ACR_LATENCY) == 0);
  // TODO: Enable UART and print out the amount of time it took for the while
  // loop to exit (a la snippets-library)
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0);
  RCC->CFGR |= RCC_CFGR_PLLSRC_HSI | RCC_CFGR_PLLMUL4 | RCC_CFGR_PLLDIV2;
  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0);
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

  SysTick_Config(32000);
}

void init_leds(void) {
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
  GPIOB->MODER ^= GPIO_MODER_MODE3_1;
}

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
    tx_thread_sleep(1000);
  }
}

void toggle_off(uint32_t thread_input) {
  while (1) {
    tx_thread_sleep(500);
    GPIOB->BSRR = GPIO_BSRR_BR_3;
    tx_thread_sleep(500);
  }
}
