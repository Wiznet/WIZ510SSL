#ifndef _MEMORY_DEFINE_H
#define _MEMORY_DEFINE_H

#define USE_SINGLE_BANK 1
#define USE_SECURE_FLASH 1

#if USE_SINGLE_BANK
#define FLASH_USE_BANK_SIZE 0x00030000
#define FLASH_BANK_PAGE_SIZE 0x1000 // page size is 4KB in single bank
#else
#define FLASH_BANK_PAGE_SIZE 0x0800 // page size is 2KB in dual bank
#endif

#define FLASH_START_ADDR_BANK0    FLASH_BASE_NS + 0x00010000 //0x08000000
#define FLASH_START_ADDR_BANK1    FLASH_START_ADDR_BANK0 + FLASH_USE_BANK_SIZE


#define FLASH_END_ADDR 0x0807FFFF
#define FLASH_REMAIN_ADDR 0x08070000

/* WIZ510SSL Application memory map */
// STM32L552CE6T
/*
 * Internal Flash
 *  - Main flash size: 512kB
 *  - 512 OTP (one-time programmable) bytes for user data
 *
 Top Flash Memory address /-------------------------------------------\  0x08080000
                          |                                           |
                          |                 Remain (64kb)             |
                          |-------------------------------------------|  0x08070000
                          |                                           |
                          |           Application Bank 1 (192kb)      |
                          |                                           |
                          |                                           |
                          |-------------------------------------------|  0x08040000
                          |                                           |
                          |                                           |
                          |           Application Bank 0 (192kb)      |
                          |                                           |
                          |                                           |
                          |-------------------------------------------|  0x08010000
    Page   1 (4KB)        |                                           |
                          |                                           |
                          |              Callable API (4KB)           |  0x0800F000
    Page   0 (4KB)        |              Secure Area (60KB)           |
                          |                                           |
                          \-------------------------------------------/  0x08000000
*/

#endif //_MEMORY_DEFINE_H
