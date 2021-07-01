################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Secure_S2E_SSL_Platform/securePlatformHandler/secureStorageHandler.c 

OBJS += \
./Secure_S2E_SSL_Platform/securePlatformHandler/secureStorageHandler.o 

C_DEPS += \
./Secure_S2E_SSL_Platform/securePlatformHandler/secureStorageHandler.d 


# Each subdirectory must supply rules for building sources it contributes
Secure_S2E_SSL_Platform/securePlatformHandler/secureStorageHandler.o: ../Secure_S2E_SSL_Platform/securePlatformHandler/secureStorageHandler.c Secure_S2E_SSL_Platform/securePlatformHandler/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Secure_S2E_SSL_Platform/securePlatformHandler/secureStorageHandler.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

