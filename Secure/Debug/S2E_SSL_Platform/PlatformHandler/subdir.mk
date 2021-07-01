################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/PlatformHandler/flashHandler.c \
C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/PlatformHandler/storageHandler.c \
C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/PlatformHandler/wiz_debug.c 

OBJS += \
./S2E_SSL_Platform/PlatformHandler/flashHandler.o \
./S2E_SSL_Platform/PlatformHandler/storageHandler.o \
./S2E_SSL_Platform/PlatformHandler/wiz_debug.o 

C_DEPS += \
./S2E_SSL_Platform/PlatformHandler/flashHandler.d \
./S2E_SSL_Platform/PlatformHandler/storageHandler.d \
./S2E_SSL_Platform/PlatformHandler/wiz_debug.d 


# Each subdirectory must supply rules for building sources it contributes
S2E_SSL_Platform/PlatformHandler/flashHandler.o: C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/PlatformHandler/flashHandler.c S2E_SSL_Platform/PlatformHandler/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"S2E_SSL_Platform/PlatformHandler/flashHandler.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
S2E_SSL_Platform/PlatformHandler/storageHandler.o: C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/PlatformHandler/storageHandler.c S2E_SSL_Platform/PlatformHandler/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"S2E_SSL_Platform/PlatformHandler/storageHandler.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
S2E_SSL_Platform/PlatformHandler/wiz_debug.o: C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/PlatformHandler/wiz_debug.c S2E_SSL_Platform/PlatformHandler/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"S2E_SSL_Platform/PlatformHandler/wiz_debug.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

