// ports/bare-arm/mphalport.c

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "stm32f4xx_hal.h" //Should include this vera

extern UART_HandleTypeDef huart2; // Assuming this is declared in main.c or a uart.c

void mp_hal_stdout_tx_char(char c) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&c, 1, HAL_MAX_DELAY);
}

int mp_hal_stdin_rx_chr(void) {
    uint8_t c;
    HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY);
    return c;
}

void mp_hal_delay_ms(mp_uint_t ms) {
    HAL_Delay(ms);
}
