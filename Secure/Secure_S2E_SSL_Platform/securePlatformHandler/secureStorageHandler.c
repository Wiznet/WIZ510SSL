
#include <string.h>
#include "flashHandler.h"
#include "deviceHandler.h"
#include "secureStorageHandler.h"
#include "memory_define.h"
#include "mbedtls/aes.h"

#include "wiz_debug.h"

#ifdef _STORAGE_DEBUG_
    #include <stdio.h>
#endif

#ifdef __USE_EXT_EEPROM__
    #include "eepromHandler.h"
    uint16_t convert_eeprom_addr(uint32_t flash_addr);
#endif

#define AES_BUF_SIZE 4096

uint8_t aes128_key[16];
uint8_t aes_buffer[AES_BUF_SIZE];

void key_init(void)
{
	//it is dummy key, Users have to create their own key.
	memset(aes128_key, 0x11, 4);
	memset(&aes128_key[4], 0x22, 4);
	memset(&aes128_key[8], 0x33, 4);
	memset(&aes128_key[12], 0x44, 4);
}

uint8_t *key_get(void)
{
    return aes128_key;
}

uint32_t cal_enc_len(uint32_t src_len)
{
    uint32_t a = (src_len % 16);
    
    if (a)
        return (16 - a + src_len);
    else
        return src_len;
}

int encrypt_aes_128(uint8_t *data_in, uint32_t data_len)
{
    mbedtls_aes_context wiz_aes;
    uint8_t *key_ptr = key_get();
    int ret;
    uint32_t enc_len;
    //uint8_t *aes_output;

    unsigned char iv[16]={0xb2, 0x4b, 0xf2, 0xf7, 0x7a, 0xc5, 0xec, 0x0c, 0x5e, 0x1f, 0x4d, 0xc1, 0xae, 0x46, 0x5e, 0x75};

    mbedtls_aes_init(&wiz_aes);
    ret = mbedtls_aes_setkey_enc(&wiz_aes, key_ptr, 128);
    if (ret < 0)
    {
        mbedtls_aes_free(&wiz_aes);
        return -1;
    }
   
    memset(aes_buffer, 0x00, AES_BUF_SIZE);
    enc_len = cal_enc_len(data_len);
    
    ret = mbedtls_aes_crypt_cbc(&wiz_aes, MBEDTLS_AES_ENCRYPT, enc_len, iv, data_in, aes_buffer);
    if (ret < 0)
    {
        mbedtls_aes_free(&wiz_aes);
        return -1;
    }

    mbedtls_aes_free(&wiz_aes);
    return ret;
}

int decrypt_aes_128(uint8_t *data_in, uint32_t data_len)
{
    mbedtls_aes_context wiz_aes;
    uint8_t *key_ptr = key_get();
    uint32_t enc_len;
    int ret;

    unsigned char iv[16]={0xb2, 0x4b, 0xf2, 0xf7, 0x7a, 0xc5, 0xec, 0x0c, 0x5e, 0x1f, 0x4d, 0xc1, 0xae, 0x46, 0x5e, 0x75};

    //aes_output = malloc(4096);
    mbedtls_aes_init(&wiz_aes);
    ret = mbedtls_aes_setkey_dec(&wiz_aes, key_ptr, 128);
    if (ret < 0)
    {
        mbedtls_aes_free(&wiz_aes);
        return -1;
    }
    memset(aes_buffer, 0x00, AES_BUF_SIZE);
    enc_len = cal_enc_len(data_len);
    
    ret = mbedtls_aes_crypt_cbc(&wiz_aes, MBEDTLS_AES_DECRYPT, enc_len, iv, data_in, aes_buffer);
    if (ret < 0)
    {
        mbedtls_aes_free(&wiz_aes);
        return -1;
    }
    mbedtls_aes_free(&wiz_aes);
    return ret;
}


int secure_read_storage(teDATASTORAGE stype, void *data, uint16_t size)
{
    int ret = -1;
   
    switch(stype)
    {
        case STORAGE_MAC:
            //ret = read_flash(OTP_BASE, data, size);
            break;
        
        case STORAGE_CONFIG:
            ret = decrypt_aes_128(S_FLASH_DEV_INFO_ADDR, size);
            memcpy(data, aes_buffer, size);
            break;

        case STORAGE_ROOTCA:
            ret = decrypt_aes_128(S_FLASH_ROOTCA_ADDR, size);
            memcpy(data, aes_buffer, size);
            break;

        case STORAGE_CLICA:
            ret = decrypt_aes_128(S_FLASH_CLICA_ADDR, size);
            memcpy(data, aes_buffer, size);
            break;  
        
        case STORAGE_PKEY:
            ret = decrypt_aes_128(S_FLASH_PKEY_ADDR, size);
            memcpy(data, aes_buffer, size);
            break;  
            
        default:
            break;
    }
    
    return ret;
}


int secure_write_storage(teDATASTORAGE stype, void *data, uint16_t size)
{
    int ret = -1;
    
    switch(stype)
    {
        case STORAGE_MAC:
            //ret = write_flash(OTP_BASE, data, size);
            break;
        
        case STORAGE_CONFIG:
            ret = encrypt_aes_128(data, size);
            if (ret < 0)
                break;
            ret = write_flash(S_FLASH_DEV_INFO_ADDR, aes_buffer, 4096);
            break;

        
        case STORAGE_ROOTCA:
            ret = encrypt_aes_128(data, size);
            if (ret < 0)
                break;
            ret = write_flash(S_FLASH_ROOTCA_ADDR, aes_buffer, 4096);
            break;

        case STORAGE_CLICA:
            ret = encrypt_aes_128(data, size);
            if (ret < 0)
                break;
            ret = write_flash(S_FLASH_CLICA_ADDR, aes_buffer, 4096);
            break;  
        
        case STORAGE_PKEY:
            ret = encrypt_aes_128(data, size);
            if (ret < 0)
                break;
            ret = write_flash(S_FLASH_PKEY_ADDR, aes_buffer, 4096);
            break;  

        default:
            break;
    }
    
    return ret;
}

int secure_erase_storage(teDATASTORAGE stype)
{
    uint16_t i;
    uint32_t address, working_address;
    
    uint8_t blocks = 0;
    uint16_t sectors = 0, remainder = 0;

    int ret = -1;
    
    switch(stype)
    {
        case STORAGE_MAC:
            printf("can't erase MAC in f/w, use stm32cube programmer\r\n");
            break;
        
        case STORAGE_CONFIG:
            ret = erase_flash_page(S_FLASH_DEV_INFO_ADDR);
#if !USE_SINGLE_BANK
            if (ret < 0 )
                return ret;
            ret = erase_flash_page(S_FLASH_DEV_INFO_ADDR + FLASH_BANK_PAGE_SIZE);
#endif
            break;

        case STORAGE_ROOTCA:
            ret = erase_flash_page(S_FLASH_ROOTCA_ADDR);
#if !USE_SINGLE_BANK
            if (ret < 0 )
                return ret;
            ret = erase_flash_page(S_FLASH_ROOTCA_ADDR + FLASH_BANK_PAGE_SIZE);
#endif
            break;

        case STORAGE_CLICA:
            ret = erase_flash_page(S_FLASH_CLICA_ADDR);
#if !USE_SINGLE_BANK
            if (ret < 0 )
                return ret;
            ret = erase_flash_page(S_FLASH_CLICA_ADDR + FLASH_BANK_PAGE_SIZE);
#endif
            break;  
        
        case STORAGE_PKEY:
            ret = erase_flash_page(S_FLASH_PKEY_ADDR);
#if !USE_SINGLE_BANK
            if (ret < 0 )
                return ret;
            ret = erase_flash_page(S_FLASH_PKEY_ADDR + FLASH_BANK_PAGE_SIZE);
#endif
            break;
        
        default:
            break;
    }
    return ret;
}

#ifdef __USE_EXT_EEPROM__
uint16_t convert_eeprom_addr(uint32_t flash_addr)
{
    return (uint16_t)(flash_addr-DAT0_START_ADDR);
}
#endif

