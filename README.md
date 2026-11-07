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

## USB shell commands

```text
help  - show available commands
prox  - read the VCNL4040 proximity value
snap  - capture and send one JPEG
```

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
