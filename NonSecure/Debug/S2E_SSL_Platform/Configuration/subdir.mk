################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/ConfigData.c \
C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/netbios.c \
C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/segcp.c \
C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/util.c 

OBJS += \
./S2E_SSL_Platform/Configuration/ConfigData.o \
./S2E_SSL_Platform/Configuration/netbios.o \
./S2E_SSL_Platform/Configuration/segcp.o \
./S2E_SSL_Platform/Configuration/util.o 

C_DEPS += \
./S2E_SSL_Platform/Configuration/ConfigData.d \
./S2E_SSL_Platform/Configuration/netbios.d \
./S2E_SSL_Platform/Configuration/segcp.d \
./S2E_SSL_Platform/Configuration/util.d 


# Each subdirectory must supply rules for building sources it contributes
S2E_SSL_Platform/Configuration/ConfigData.o: C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/ConfigData.c S2E_SSL_Platform/Configuration/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Internet/DHCP -I../../ioLibrary_Driver/Internet/DNS -I../../ioLibrary_Driver/Internet/MQTT/MQTTPacket -I../../ioLibrary_Driver/Internet/MQTT -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../ioLibrary_Driver/Ethernet/W5100S -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Callback -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/Serial_to_Ethernet -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"S2E_SSL_Platform/Configuration/ConfigData.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
S2E_SSL_Platform/Configuration/netbios.o: C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/netbios.c S2E_SSL_Platform/Configuration/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Internet/DHCP -I../../ioLibrary_Driver/Internet/DNS -I../../ioLibrary_Driver/Internet/MQTT/MQTTPacket -I../../ioLibrary_Driver/Internet/MQTT -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../ioLibrary_Driver/Ethernet/W5100S -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Callback -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/Serial_to_Ethernet -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"S2E_SSL_Platform/Configuration/netbios.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
S2E_SSL_Platform/Configuration/segcp.o: C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/segcp.c S2E_SSL_Platform/Configuration/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Internet/DHCP -I../../ioLibrary_Driver/Internet/DNS -I../../ioLibrary_Driver/Internet/MQTT/MQTTPacket -I../../ioLibrary_Driver/Internet/MQTT -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../ioLibrary_Driver/Ethernet/W5100S -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Callback -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/Serial_to_Ethernet -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"S2E_SSL_Platform/Configuration/segcp.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"
S2E_SSL_Platform/Configuration/util.o: C:/Users/Hoon-Mac/Documents/Work/Project/S2E_SSL/STM32CubeIDE/workspace_1.5.0/WIZ510SSL/S2E_SSL_Platform/Configuration/util.c S2E_SSL_Platform/Configuration/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m33 -std=gnu11 -g3 -DUSE_HAL_DRIVER -DSTM32L552xx -DDEBUG '-DMBEDTLS_CONFIG_FILE=<SSLConfig.h>' -c -I../Core/Inc -I../../Secure_nsclib -I../../Drivers/STM32L5xx_HAL_Driver/Inc -I../../Drivers/CMSIS/Device/ST/STM32L5xx/Include -I../../Drivers/STM32L5xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Include -I../../ioLibrary_Driver -I../../ioLibrary_Driver/Ethernet -I../../ioLibrary_Driver/Internet/DHCP -I../../ioLibrary_Driver/Internet/DNS -I../../ioLibrary_Driver/Internet/MQTT/MQTTPacket -I../../ioLibrary_Driver/Internet/MQTT -I../../ioLibrary_Driver/Security/SSLTLS -I../../ioLibrary_Driver/Security/SSLTLS/mbedtls/include -I../../ioLibrary_Driver/Ethernet/W5100S -I../../S2E_SSL_Platform -I../../S2E_SSL_Platform/Board -I../../S2E_SSL_Platform/Callback -I../../S2E_SSL_Platform/Configuration -I../../S2E_SSL_Platform/Serial_to_Ethernet -I../../S2E_SSL_Platform/PlatformHandler -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"S2E_SSL_Platform/Configuration/util.d" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

