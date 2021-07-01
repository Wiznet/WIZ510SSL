#ifndef STORAGEHANDLER_H_
#define STORAGEHANDLER_H_

#include <stdint.h>
#include "memory_define.h"
#define _STORAGE_DEBUG_

typedef enum {
    STORAGE_MAC,
    STORAGE_CONFIG, 
    STORAGE_APP_MAIN, 
    STORAGE_APP_BACKUP, 
    NETWORK_APP_BACKUP,
    SERVER_APP_BACKUP, 
    STORAGE_APPBOOT,
    STORAGE_APPBANK0,
    STORAGE_APPBANK1,
    STORAGE_ROOTCA, 
    STORAGE_CLICA,
    STORAGE_PKEY
} teDATASTORAGE;

#define FLASH_DEV_INFO_ADDR FLASH_REMAIN_ADDR  //hoon Temporary
#define FLASH_ROOTCA_ADDR   FLASH_DEV_INFO_ADDR + 0x1000
#define FLASH_CLICA_ADDR    FLASH_ROOTCA_ADDR + 0x1000
#define FLASH_PRIKEY_ADDR   FLASH_CLICA_ADDR + 0x1000
#define FLASH_MAC_ADDR      FLASH_PRIKEY_ADDR + 0x1000

uint32_t read_storage(teDATASTORAGE stype, uint32_t addr, void *data, uint16_t size);
uint32_t write_storage(teDATASTORAGE stype, uint32_t addr, void *data, uint16_t size);
void erase_storage(teDATASTORAGE stype);

#endif /* STORAGEHANDLER_H_ */
