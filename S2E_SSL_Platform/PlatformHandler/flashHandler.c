#include <string.h>
#include "stm32l5xx_hal.h"
#include "common.h"
#include "flashHandler.h"
#include "wiz_debug.h"
#include "memory_define.h"

#ifdef _FLASH_DEBUG_
    #include <stdio.h>
    #include <ctype.h>
#endif


static uint32_t GetSector(uint32_t Address);
static uint32_t GetSectorSize(uint32_t Sector);
static uint32_t GetSectorStartAddr(uint32_t Sector);


uint32_t write_flash(uint32_t addr, uint8_t * data, uint32_t data_len)
{

    uint32_t i;
    uint32_t rep_word = 0, rep_byte = 0;

    uint8_t byte_data;
    uint64_t double_word_data;
#if 1
    __disable_irq();

    if(data_len)
    {
        rep_word = (data_len / 8);
        rep_byte = (data_len % 8);
    }

    /* Enable the flash control register access */
    HAL_FLASH_Unlock();

    /* Clear pending flags (if any) */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ECCR_ERRORS | FLASH_FLAG_OPTWERR | FLASH_FLAG_ECCR_ERRORS | FLASH_FLAG_OPTWERR);

    for(i = 0 ; i < rep_word ; i++)
    {
        double_word_data = *((uint64_t *)data + i);
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, double_word_data) == HAL_OK)
        {
#if 0
            if(*(uint64_t *)(addr) != (double_word_data)) // verify failed
            {
                PRT_ERR("*(uint64_t *)(addr) = 0x%16X, double_word_data = 0x%16X\r\n", *(uint64_t *)(addr), double_word_data);
                return -2;
            }
#endif
        }
        else
        {
            return -1; // flash write failed
        }
        addr += 8;
    }

    if(rep_byte)
    {
        double_word_data = *((uint64_t *)data + rep_word);
        double_word_data |= (0xffffffffffffffff << (8 * rep_byte)); // padding remain bytes to 0xff

        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr, double_word_data) != HAL_OK)
        {
            return -1; // flash write failed
        }
        addr += rep_byte;
    }
    HAL_FLASH_Lock();

    __enable_irq();
#endif
    return data_len;
}

uint32_t read_flash(uint32_t addr, uint8_t *data, uint32_t data_len)
{
    uint32_t i = 0;
    uint32_t read_len = 0;

//    if((addr + data_len) > FLASH_END_ADDR) return 0;

    for(i = 0; i < data_len; i++)
    {
        data[i] = *(uint8_t *) (addr + i);
        read_len++;
    }

    return read_len;
}


int8_t erase_flash_page(uint32_t addr)
{
    uint32_t PageError;
    int ret;
    FLASH_EraseInitTypeDef pEraseInit;
    
    __disable_irq();

    /* Enable the flash control register access */
    HAL_FLASH_Unlock();

    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    pEraseInit.Banks = GetBank(addr);
    pEraseInit.NbPages = 1;
    pEraseInit.Page = GetPage(addr);// (addr - FLASH_START_ADDR_BANK2) / FLASH_PAGE_SIZE;

    ret = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
    if(ret != HAL_OK )
    {
        HAL_FLASH_Lock();
        __enable_irq();
        PRT_ERR("ret = %d\r\n", ret);
        return -1;
    }

    /* Lock the Flash to disable the flash control register access (recommended
       to protect the FLASH memory against possible unwanted operation) */
    HAL_FLASH_Lock();

    __enable_irq();

    return 1;

}

uint32_t GetPage(uint32_t Addr)
{
    uint32_t page = 0;

#if USE_SINGLE_BANK
    page = (Addr - FLASH_BASE) / FLASH_BANK_PAGE_SIZE;
#else
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
    }
    else
    {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
    }
#endif
    PRT_INFO("page = %d\r\n", page);

    return page;
}

uint32_t GetBank(uint32_t Addr)
{
    return FLASH_BANK_1;
    
#if USE_SINGLE_BANK

#else
    if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
    {
        /* Bank 1 */
        return FLASH_BANK_1;
    }
    else
    {
        /* Bank 2 */
        return FLASH_BANK_2;
        
    }
#endif
}


#ifdef _FLASH_DEBUG_
void dump_flash_sector(uint32_t addr)
{
#if 1
    uint16_t i, j;
    uint16_t len;
    uint8_t buf[DATA_BUF_SIZE];

    uint16_t cnt;
    uint32_t UserStartSector, UserStartAddr, UserSectorSize;
    uint32_t UpdateStartAddr;
    uint8_t rep = 0; // memory read repetition count

    UserStartSector = GetSector(addr);
    UserStartAddr = GetSectorStartAddr(UserStartSector);
    UserSectorSize = GetSectorSize(UserStartSector);

    rep = UserSectorSize / DATA_BUF_SIZE;

    if(IS_FLASH_ADDRESS(addr))
    {
        printf("\r\n > FLASH:DUMP: [0x%.8x ~ 0x%.8x], FLASH_SECTOR_%d\r\n", UserStartAddr, (UserStartAddr + UserSectorSize - 1), UserStartSector);
    }
    else
    {
        printf("\r\n > FLASH:DUMP: Failed [0x%.8x ~ 0x%.8x], FLASH_SECTOR_%d\r\n", UserStartAddr, (UserStartAddr + UserSectorSize - 1), UserStartSector);
        return;
    }


    for(cnt = 0; cnt < rep; cnt++)
    {
        UpdateStartAddr = UserStartAddr + (cnt * DATA_BUF_SIZE);
        len = read_flash(UpdateStartAddr, buf, DATA_BUF_SIZE);

        printf("\r\n > FLASH:DUMP: [0x%.8x ~ 0x%.8x], FLASH_SECTOR_%d [%d/%d]\r\n", UpdateStartAddr, (UpdateStartAddr + DATA_BUF_SIZE - 1), UserStartSector, cnt+1, rep);

        for(i = 0; i < len; i++)
        {
            if((i%16) == 0) printf("0x%.08x : ", (i + UpdateStartAddr));
            printf("%.02x ", buf[i]);

            if((i%16-15) == 0)
            {
                printf("  ");
                for (j = i-15; j <= i; j++)
                {
                    if(isprint(buf[j]))
                        printf("%c", buf[j]);
                    else
                        printf(".");
                }
                printf("\r\n");
            }
        }
        printf("\r\n");
    }
#endif
}

#endif


