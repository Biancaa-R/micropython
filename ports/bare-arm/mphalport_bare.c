// File: ports/bare-arm/mphalport.c

#include "stm32f4xx.h"  // or your own CMSIS-style header

void uart_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    GPIOA->MODER &= ~(3 << (9 * 2));
    GPIOA->MODER |= (2 << (9 * 2));
    GPIOA->AFR[1] |= (7 << ((9 - 8) * 4));

    USART1->BRR = 0x0683; // 9600 baud @ 16 MHz
    USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void mp_hal_stdout_tx_char(char c) {
    while (!(USART1->SR & USART_SR_TXE)) {
    }
    USART1->DR = (uint8_t)c;
}
//Tx only for checking
