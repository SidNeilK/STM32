# STM32

Integrated embedded example for an **Espressif ESP32 development board** using:

- **VL53L0X** proximity sensor (I2C)
- **SSD1306 OLED** display (I2C)
- **ArduCAM Mini OV2640** camera (SPI + I2C control)

## Example

- Sketch: `examples/esp32_proximity_oled_arducam/esp32_proximity_oled_arducam.ino`

Behavior:

1. Continuously reads proximity distance
2. Shows live status on the OLED
3. Triggers an ArduCAM capture when distance is below 250 mm (with cooldown)

## Arduino IDE / PlatformIO Libraries

Install these libraries before building:

- `ArduCAM`
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `VL53L0X` (Pololu)

## Typical ESP32 Wiring

- OLED and VL53L0X on I2C (`SDA`/`SCL`)
- ArduCAM SPI: `CS=GPIO5`, plus `SCK/MISO/MOSI`
- Status LED: `GPIO2`

> If compilation fails with `OV2640_MINI_2MP` errors, enable it in `ArduCAM/memorysaver.h`.
