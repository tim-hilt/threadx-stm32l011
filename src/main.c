// dep
#include "tx_api.h"
// local
#include "stm32l0xx.h"

TX_THREAD toggle_on_thread;
TX_THREAD toggle_off_thread;

uint8_t buffer[32];

void init_uart(void);
void init_leds(void);
void toggle_on(uint32_t);
void toggle_off(uint32_t);
void increase_clock_speed(void);
void UnlockPELOCK(void);
void UnlockPRGLOCK(void);

#define FLASH_PEKEY1 (0x89ABCDEFU)
#define FLASH_PEKEY2 (0x02030405U)
#define FLASH_PRGKEY1 (0x8C9DAEBFU)
#define FLASH_PRGKEY2 (0x13141516U)

extern uint32_t _edata;

int main(void) {
  init_leds();
  init_uart();
  increase_clock_speed();
  UnlockPELOCK();
  UnlockPRGLOCK();

  tx_kernel_enter();
}

__INLINE void UnlockPELOCK(void) {
  while ((FLASH->SR & FLASH_SR_BSY) != 0);
  if ((FLASH->PECR & FLASH_PECR_PELOCK) != 0) {
    FLASH->PEKEYR = FLASH_PEKEY1;
    FLASH->PEKEYR = FLASH_PEKEY2;
  }
}

__INLINE void UnlockPRGLOCK(void) {
  while ((FLASH->SR & FLASH_SR_BSY) != 0);
  if ((FLASH->PECR & FLASH_PECR_PELOCK) == 0) {
    if ((FLASH->PECR & FLASH_PECR_PRGLOCK) != 0) {
      FLASH->PRGKEYR = FLASH_PRGKEY1;
      FLASH->PRGKEYR = FLASH_PRGKEY2;
    }
  }
}

void FlashWord32Prog(uint32_t flash_addr, uint32_t data) {
  *(__IO uint32_t*)(flash_addr) = data;

  while ((FLASH->SR & FLASH_SR_BSY) != 0);

  if ((FLASH->SR & FLASH_SR_EOP) != 0) {
    FLASH->SR = FLASH_SR_EOP;
  } else if ((FLASH->SR & FLASH_SR_FWWERR) != 0) {
    FLASH->SR = FLASH_SR_FWWERR;
  } else if ((FLASH->SR & FLASH_SR_NOTZEROERR) != 0) {
    FLASH->SR = FLASH_SR_NOTZEROERR;
  } else if ((FLASH->SR & FLASH_SR_SIZERR) != 0) {
    FLASH->SR = FLASH_SR_SIZERR;
  } else if ((FLASH->SR & FLASH_SR_WRPERR) != 0) {
    FLASH->SR = FLASH_SR_WRPERR;
  }
}

// TODO: Try to understand every step that's done here
__INLINE void increase_clock_speed(void) {
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

__INLINE void init_leds(void) {
  RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
  GPIOB->MODER ^= GPIO_MODER_MODE3_1;
}

__INLINE void init_uart(void) {
  RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

  GPIOA->MODER &= ~GPIO_MODER_MODE2_Msk;
  GPIOA->MODER |= 0b10 << GPIO_MODER_MODE2_Pos;

  GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2_Msk;
  GPIOA->AFR[0] |= 4U << GPIO_AFRL_AFSEL2_Pos;

  RCC->AHBENR |= RCC_AHBENR_DMA1EN;

  DMA1_CSELR->CSELR = 4 << DMA_CSELR_C4S_Pos;
  DMA1_Channel4->CPAR = (uint32_t)&USART2->TDR;
  DMA1_Channel4->CMAR = (uint32_t)buffer;
  DMA1_Channel4->CCR = DMA_CCR_MINC | DMA_CCR_DIR;

  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

  RCC->CCIPR &= ~RCC_CCIPR_USART2SEL_Msk;

  USART2->BRR = 32000000 / 115200;

  USART2->CR3 = USART_CR3_DMAT;

  USART2->CR1 |= USART_CR1_TE | USART_CR1_UE;
}

void print(const char* str) {
  if (strlen(str) > sizeof(buffer)) {
    return;
  }
  for (int i = 0; i < strlen(str); i++) {
    buffer[i] = str[i];
  }
  DMA1_Channel4->CCR &= ~DMA_CCR_EN;
  DMA1_Channel4->CNDTR = strlen(str);
  DMA1_Channel4->CCR |= DMA_CCR_EN;

  while (!(DMA1->ISR & DMA_ISR_TCIF4));
  DMA1->IFCR = DMA_IFCR_CTCIF4;
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
    print("on\n");
    FlashWord32Prog((uint32_t)&_edata, *((uint32_t*)&_edata) + 1);
    tx_thread_sleep(1000);
  }
}

#define UINT32_DIVISOR 1000000000U

void toggle_off(uint32_t thread_input) {
  uint32_t data;
  char c[1];
  uint32_t divisor = UINT32_DIVISOR;

  while (1) {
    tx_thread_sleep(500);
    GPIOB->BSRR = GPIO_BSRR_BR_3;
    print("off\n");

    data = *((uint32_t*)&_edata);

    while (divisor > 0) {
      c[0] = '0' + ((data / divisor) % 10);
      print(c);
      divisor /= 10;
    }
    print("\n");
    divisor = UINT32_DIVISOR;

    tx_thread_sleep(500);
  }
}
