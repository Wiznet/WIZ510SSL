/*
 * wiz_debug.c
 *
 *  Created on: Jan 19, 2021
 *      Author: Mason
 */

#include "stm32l5xx_hal.h"
#include "wiz_debug.h"
#include <stdio.h>
#include <stdarg.h>

#if !USE_SECURE_PRINTF
//extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart2;


#define RETARGET_PRINTF_USART           huart2 // Debug UART

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
    #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
    #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

PUTCHAR_PROTOTYPE
{
    /* Implement your write code here, this is used by puts and printf for example */

    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */

    uint8_t c[1];
    c[0] = ch & 0x00FF;
    HAL_UART_Transmit(&RETARGET_PRINTF_USART, &c[0], 1, 10);
    return ch;
}

#endif

#if USE_SECURE_PRINTF
void WIZ_SECURE_printf(const char *format, ...)
{
	char va_buf[2048] = {0, };
    
	va_list ap;
	va_start(ap, format);
	//vprintf(format, ap);
	//printf(format, ##__VA_ARGS__);

	vsprintf(va_buf, format, ap);
	SECURE_printf(va_buf);
	va_end(ap);
}
#endif


void PRT_DUMP(char *p, uint32_t len)
{
    uint32_t i;

    for(i=0; i<len; i++)
    {
        SECURE_debug("%02X ", *(p+i));
    }
    SECURE_debug("\r\n");

}



