#include <Wire.h>
#include <SPI.h>
#include <ArduCAM.h>
#include <memorysaver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <VL53L0X.h>

#if !defined(OV2640_MINI_2MP)
#error "Enable OV2640_MINI_2MP in ArduCAM/memorysaver.h"
#endif

namespace {
constexpr uint8_t OLED_WIDTH = 128;
constexpr uint8_t OLED_HEIGHT = 64;
constexpr int8_t OLED_RESET_PIN = -1;
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

constexpr uint8_t ARDUCAM_CS_PIN = 5;
constexpr uint8_t STATUS_LED_PIN = 2;

constexpr uint16_t PROXIMITY_TRIGGER_MM = 250;
constexpr uint32_t CAPTURE_COOLDOWN_MS = 3000;

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET_PIN);
VL53L0X tof;
ArduCAM camera(OV2640, ARDUCAM_CS_PIN);

uint32_t captureCount = 0;
uint32_t lastCaptureMs = 0;
}

void drawStatus(uint16_t distanceMm, bool capturedNow) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("ESP32 Sensor Hub"));
  display.println();

  display.print(F("Distance: "));
  if (distanceMm == 0 || distanceMm == 65535) {
    display.println(F("N/A"));
  } else {
    display.print(distanceMm);
    display.println(F(" mm"));
  }

  display.print(F("Captures: "));
  display.println(captureCount);

  display.print(F("State: "));
  display.println(capturedNow ? F("Captured") : F("Monitoring"));
  display.display();
}

bool initCamera() {
  pinMode(ARDUCAM_CS_PIN, OUTPUT);
  digitalWrite(ARDUCAM_CS_PIN, HIGH);

  camera.write_reg(ARDUCHIP_TEST1, 0x55);
  if (camera.read_reg(ARDUCHIP_TEST1) != 0x55) {
    return false;
  }

  camera.set_format(JPEG);
  camera.InitCAM();
  camera.OV2640_set_JPEG_size(OV2640_640x480);
  camera.clear_fifo_flag();
  return true;
}

bool captureFrame() {
  camera.flush_fifo();
  camera.clear_fifo_flag();
  camera.start_capture();

  const uint32_t timeoutStart = millis();
  while (!(camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))) {
    if (millis() - timeoutStart > 3000) {
      return false;
    }
    delay(10);
  }

  const uint32_t frameLength = camera.read_fifo_length();
  if (frameLength == 0 || frameLength > 0x7FFFF) {
    camera.clear_fifo_flag();
    return false;
  }

  camera.clear_fifo_flag();
  ++captureCount;
  return true;
}

void setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);

  Serial.begin(115200);
  Wire.begin();
  SPI.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS)) {
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Starting..."));
  display.display();

  if (!tof.init()) {
    display.println(F("VL53L0X failed"));
    display.display();
    while (true) {
      delay(1000);
    }
  }

  tof.setTimeout(500);
  tof.startContinuous(60);

  if (!initCamera()) {
    display.println(F("ArduCAM failed"));
    display.display();
    while (true) {
      delay(1000);
    }
  }

  display.println(F("Ready"));
  display.display();
  delay(300);
}

void loop() {
  const uint16_t distanceMm = tof.readRangeContinuousMillimeters();

  bool capturedNow = false;
  const bool inRange = (distanceMm > 0 && distanceMm < PROXIMITY_TRIGGER_MM);
  if (inRange && (millis() - lastCaptureMs > CAPTURE_COOLDOWN_MS)) {
    capturedNow = captureFrame();
    if (capturedNow) {
      lastCaptureMs = millis();
      Serial.print(F("Captured frame #"));
      Serial.println(captureCount);
      digitalWrite(STATUS_LED_PIN, HIGH);
      delay(50);
      digitalWrite(STATUS_LED_PIN, LOW);
    }
  }

  drawStatus(distanceMm, capturedNow);
  delay(120);
}
