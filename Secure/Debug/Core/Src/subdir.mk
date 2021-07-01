################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/main.c \
../Core/Src/secure_nsc.c \
../Core/Src/stm32l5xx_hal_msp.c \
../Core/Src/stm32l5xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32l5xx_s.c 

OBJS += \
./Core/Src/main.o \
./Core/Src/secure_nsc.o \
./Core/Src/stm32l5xx_hal_msp.o \
./Core/Src/stm32l5xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32l5xx_s.o 

C_DEPS += \
./Core/Src/main.d \
./Core/Src/secure_nsc.d \
./Core/Src/stm32l5xx_hal_msp.d \
./Core/Src/stm32l5xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32l5xx_s.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/main.o: ../Core/Src/main.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Core/Src/main.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/secure_nsc.o: ../Core/Src/secure_nsc.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Core/Src/secure_nsc.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/stm32l5xx_hal_msp.o: ../Core/Src/stm32l5xx_hal_msp.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Core/Src/stm32l5xx_hal_msp.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/stm32l5xx_it.o: ../Core/Src/stm32l5xx_it.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Core/Src/stm32l5xx_it.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/syscalls.o: ../Core/Src/syscalls.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Core/Src/syscalls.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/sysmem.o: ../Core/Src/sysmem.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Core/Src/sysmem.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
Core/Src/system_stm32l5xx_s.o: ../Core/Src/system_stm32l5xx_s.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I"../Secure_S2E_SSL_Platform/securePlatformHandler" -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -mcmse -MMD -MP -MF"Core/Src/system_stm32l5xx_s.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

