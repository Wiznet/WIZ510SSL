# WIZ510SSL

WIZ510SSL is S2E module that supports SSL protocol.

# Contents

- [Development Environment](#development-environment)
- [Memory structure](#memory-structure)
- [Running Application](#running-application)
- [Firmware Writing](#firmware-writing)

# Development Environment

Following software is required to be installed on your machine:
- [STM32CubeIDE](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-ides/stm32cubeide.html) version 1.6.1 (as of May 2021)

- [STM32CubeProg](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-programmers/stm32cubeprog.html) version 2.7.0 (as of May 2021)

- [STM32CubeMX](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-configurators-and-code-generators/stm32cubemx.html) version 6.2.1 (as of May 2021)

# Memory Structure

WIZ510SSL operates two flash banks:

| Bank      | Address   |
|-----------|-----------|
| Bank 0    | 0x08010000 |
| Bank 1    | 0x08040000 |


**WIZ510SSL Application memory map**

 * Internal Flash
 *  - Main flash size: 512kB
 *  - 512 OTP (one-time programmable) bytes for user data

```c
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
```


# Running Application

After importing project into STM32CubeIDE, there will be 2 nested projects: Secure and Non-Secure.

It is necessary to compile both projects.

To assign flash bank it is necessary to update address in NonSecure STM32L552CETX_FLASH.ld as following:

| Bank      | Address   |
|-----------|-----------|
| For Bank 0    | 0x08010000 |
| For Bank 1    | 0x08040000 |

![](/img/flash_ld.png)

Please use appropriate address during firmware writing!

# Firmware Writing

To write firmware CubeProgrammer is used.
Connect your board using ST-LINK connector.

![](/img/stlink_connection.png)

Before writing firmware, option bytes shall be configured properly.

Using CubeProgrammer set as following:

- DBANK = 0 // DBANK can be disabled only when TZEN is disabled. Disable DBANK first and then enable TZEN.
- TZEN = 1
- SECWM1_PSTRT = 0
- SECWM1_PEND = 0xc
- SECWM2_PSTRT = 1
- SECWM2_PEND = 0

![](/img/cubeprogrammer_settings.png)

There are 2 ways to write firmware.

1. Using .elf file

Select appropriate Secure/Non-Secure .elf file and press "Download" button

![](/img/elf_writing.png)

2. Using .bin file

Select appropriate Secure/Non-Secure .bin files, input correct address and press "Download" button

![](/img/bin_writing.png)
