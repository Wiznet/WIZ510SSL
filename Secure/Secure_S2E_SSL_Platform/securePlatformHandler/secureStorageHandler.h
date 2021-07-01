#ifndef SECURESTORAGEHANDLER_H_
#define SECURESTORAGEHANDLER_H_

#include <stdint.h>
#include "storageHandler.h"

//#define _STORAGE_DEBUG_

#if 0
typedef enum {
    STORAGE_MAC,
    STORAGE_CONFIG, 
    STORAGE_APP_MAIN, 
    STORAGE_APP_BACKUP, 
    NETWORK_APP_BACKUP,
    SERVER_APP_BACKUP, 
    STORAGE_APPBOOT, 
    STORAGE_ROOTCA, 
    STORAGE_CLICA,
    STORAGE_PKEY
} teDATASTORAGE;
#endif

#define S_FLASH_DEV_INFO_ADDR 0x08008000
#define S_FLASH_ROOTCA_ADDR   S_FLASH_DEV_INFO_ADDR + 0x1000
#define S_FLASH_CLICA_ADDR    S_FLASH_ROOTCA_ADDR + 0x1000
#define S_FLASH_PKEY_ADDR     S_FLASH_CLICA_ADDR + 0x1000

int secure_read_storage(teDATASTORAGE stype, void *data, uint16_t size);
int secure_write_storage(teDATASTORAGE stype, void *data, uint16_t size);
int secure_erase_storage(teDATASTORAGE stype);

#endif /* SECURESTORAGEHANDLER_H_ */
