#ifndef __FLASHHANDLER_H__
#define __FLASHHANDLER_H__

#include <stdint.h>
//#define _FLASH_DEBUG_

/* STM32F411xC/E Flash memory map */
// Main flash size: 512kB
// 512 OTP (one-time programmable) bytes for user data
#if 0
#define FLASH_START_ADDR_BANK1    FLASH_BASE_NS //0x08000000
#define FLASH_START_ADDR_BANK2    FLASH_BASE_NS + FLASH_BANK_SIZE

#define FLASH_END_ADDR 0x0807FFFF
#endif

uint32_t write_flash(uint32_t addr, uint8_t *data, uint32_t data_len);
uint32_t read_flash(uint32_t addr, uint8_t *data, uint32_t data_len);
int8_t erase_flash_page(uint32_t addr);

uint32_t GetPage(uint32_t Addr);
    
uint32_t GetBank(uint32_t Addr);


#ifdef _FLASH_DEBUG_
    void dump_flash_sector(uint32_t addr);
#endif

#endif
