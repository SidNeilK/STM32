# STM-1

STM32L452RE application for the ECEN-435 embedded systems project. The project
integrates an ArduCAM OV2640, VCNL4040 proximity sensor, SSD1306 OLED, USB
command shell, and UART4 communication with an ESP32-C6 JPEG sender.

## Features

- Captures 160x120 JPEG images from the ArduCAM.
- Sends `IMG` followed by the binary JPEG over UART4.
- Reads proximity values from the VCNL4040 over I2C.
- Displays status information on the SSD1306 OLED.
- Provides `help`, `prox`, and `snap` commands through the USB shell.

## STM32-to-ESP32 wiring

| STM32L452RE | Function | ESP32-C6 |
|---|---|---|
| PA0 | UART4 TX | GPIO17 / U0RXD |
| PC11 | UART4 RX | GPIO16 / U0TXD |
| GND | Ground | GND |

UART4 is configured for 115200 baud, 8 data bits, no parity, and one stop bit.
The image packet format is:

```text
IMG + JPEG bytes
```

The JPEG begins with `FF D8` and ends with `FF D9`.

## USB shell commands

```text
help  - show available commands
prox  - read the VCNL4040 proximity value
snap  - capture and send one JPEG
```

## Build and flash

From the project directory:

```powershell
make -j4
st-flash write build/STM-1.bin 0x8000000
```

The generated firmware files are placed in `build/`, including:

- `STM-1.elf`
- `STM-1.bin`
- `STM-1.hex`

## Code organization

Project-specific application code is primarily located in:

- `Core/Src/main.c` — application flow and USB commands
- `Core/Src/proximity_sensor.c` — VCNL4040 driver
- `Core/Src/spi.c` — ArduCAM capture and JPEG transport

STM32CubeMX-generated initialization, STM32 HAL, CMSIS, and middleware files
are included as project dependencies.

The ESP32 sender and ESP8266 receiver are maintained separately from this
STM32 project. They receive the `IMG` packet, forward it over ESP-NOW, and
decode the JPEG at the receiver.
