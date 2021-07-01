
#include <string.h>
#include "common.h"
#include "s2e_ssl_board.h"
#include "flashHandler.h"
#include "deviceHandler.h"
#include "storageHandler.h"

#include "memory_define.h"
#include "wiz_debug.h"

#ifdef _STORAGE_DEBUG_
    #include <stdio.h>
#endif

#ifdef __USE_EXT_EEPROM__
    #include "eepromHandler.h"
    uint16_t convert_eeprom_addr(uint32_t flash_addr);
#endif

uint32_t read_storage(teDATASTORAGE stype, uint32_t addr, void *data, uint16_t size)
{
    uint32_t ret_len;
    
    switch(stype)
    {
        case STORAGE_MAC:
            //ret_len = read_flash(OTP_BASE, data, size);
            ret_len = read_flash(FLASH_MAC_ADDR, data, size);
            break;
        
        case STORAGE_CONFIG:
            ret_len = read_flash(FLASH_DEV_INFO_ADDR, data, size);
            break;
        
        case STORAGE_APP_MAIN:
            ret_len = read_flash(addr, data, size);
            break;
        
        case STORAGE_APP_BACKUP:
            ret_len = read_flash(addr, data, size);
            break;
        default:
            break;
    }
    
    return ret_len;
}


uint32_t write_storage(teDATASTORAGE stype, uint32_t addr, void *data, uint16_t size)
{
    uint32_t ret_len;
    
    switch(stype)
    {
        case STORAGE_MAC:
            //write_flash(OTP_BASE, data, size);
            write_flash(FLASH_MAC_ADDR, data, size);
            break;
        
        case STORAGE_CONFIG:
            write_flash(FLASH_DEV_INFO_ADDR, data, size);
            break;
        
        case STORAGE_APPBOOT:
        case STORAGE_APP_MAIN:
            //ret_len = write_flash(addr, data, size);
            break;

        case STORAGE_APP_BACKUP:
            //ret_len = write_sflash(addr, data, size); // serial flash bank for main application download / backup
            break;
        case STORAGE_ROOTCA:
            write_flash(FLASH_ROOTCA_ADDR, data, size);            
            break;
        
        case STORAGE_CLICA:
            write_flash(FLASH_CLICA_ADDR, data, size);
            break;
        case STORAGE_PKEY:
            write_flash(FLASH_PRIKEY_ADDR, data, size);
            break;

        default:
            break;
    }
    
    return ret_len;
}

void erase_storage(teDATASTORAGE stype)
{
    uint16_t i;
    uint32_t address, working_address;
    
    uint8_t blocks = 0;
    uint32_t sectors = 0, remainder = 0;
    int ret;
    
    switch(stype)
    {
        case STORAGE_MAC:
            printf("can't erase MAC in f/w, use stm32cube programmer\r\n");
            break;
        
        case STORAGE_CONFIG:
            erase_flash_page(FLASH_DEV_INFO_ADDR);
            break;
        
        case STORAGE_APP_MAIN:
            //address = DEVICE_APP_ADDR;
            break;
        
        case STORAGE_APP_BACKUP:
            //address = DEVICE_APP_ADDR;
            break;
        
        case STORAGE_APPBOOT:
            //address = DEVICE_BOOT_ADDR;
            break;

        case STORAGE_APPBANK0:
            address = FLASH_START_ADDR_BANK0;
            break;
            
        case STORAGE_APPBANK1:
            address = FLASH_START_ADDR_BANK1;
            break;

        case STORAGE_ROOTCA:
            erase_flash_page(FLASH_ROOTCA_ADDR);
            break;
        case STORAGE_CLICA:
            erase_flash_page(FLASH_CLICA_ADDR);
        default:
            break;
    }
    if((stype == STORAGE_APPBANK0) || (stype == STORAGE_APPBANK1))
    {
        working_address = address;
        sectors = FLASH_USE_BANK_SIZE / FLASH_BANK_PAGE_SIZE;

        for(i = 0; i < sectors; i++)
        {
            ret = erase_flash_page(working_address);
            PRT_INFO(" > STORAGE:SECTOR_ERASE:ADDR - 0x%x\r\n", working_address);
            working_address += FLASH_BANK_PAGE_SIZE;
        }
        //working_address += (sectors * SECT_SIZE);
        PRT_INFO(" > STORAGE:ERASE_END:ADDR_RANGE - [0x%x ~ 0x%x]\r\n", address, working_address-1);
    }
}

#ifdef __USE_EXT_EEPROM__
uint16_t convert_eeprom_addr(uint32_t flash_addr)
{
    return (uint16_t)(flash_addr-DAT0_START_ADDR);
}
#endif

