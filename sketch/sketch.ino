#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Wire.h>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789      _panel_instance;
  lgfx::Bus_SPI           _bus_instance;

public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();

      cfg.spi_host   = SPI2_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;
      cfg.freq_read  = 16000000;
      cfg.spi_3wire  = false;
      cfg.use_lock   = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;

      // NORVI X CPU-ESPS3-X1 SPI Pinout
      cfg.pin_sclk = 12;
      cfg.pin_mosi = 11;
      cfg.pin_miso = 13;
      cfg.pin_dc   = 46;

      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();

      cfg.pin_cs           = 45;
      cfg.pin_rst          = 47;
      cfg.pin_busy         = -1;

      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits  = 1;
      cfg.readable         = false;
      cfg.invert           = true;
      cfg.rgb_order        = false;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = true;

      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance);
  }
};

LGFX lcd;

// I2C Pin definitions for NORVI X CPU-ESPS3-X1
#define SDA_PIN 8
#define SCL_PIN 9
#define PCA9536_ADDR 0x41

// Previous button states
bool prevBtn1 = false;
bool prevBtn2 = false;

// Function to read inputs from PCA9536 (4-bit IO expander)
uint8_t readPCA9536() {
  Wire.beginTransmission(PCA9536_ADDR);
  Wire.write(0x00); // Input Port register
  if (Wire.endTransmission() != 0) {
    return 0xFF; // I2C communication error
  }
  Wire.requestFrom((uint8_t)PCA9536_ADDR, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0xFF;
}

void drawButtonCard(int x, int y, int w, int h, const char* label, bool pressed) {
  uint16_t boxColor = pressed ? TFT_DARKGREEN : 0x18E3;
  uint16_t borderColor = pressed ? TFT_GREEN : TFT_WHITE;
  
  lcd.fillRoundRect(x, y, w, h, 8, boxColor);
  lcd.drawRoundRect(x, y, w, h, 8, borderColor);
  
  lcd.setTextDatum(top_center);
  lcd.setTextColor(TFT_WHITE, boxColor);
  lcd.setTextSize(2);
  lcd.drawString(label, x + w / 2, y + 10);
  
  lcd.setTextDatum(middle_center);
  if (pressed) {
    lcd.setTextColor(TFT_YELLOW, boxColor);
    lcd.setTextSize(3);
    lcd.drawString("PRESSED", x + w / 2, y + h / 2 + 10);
  } else {
    lcd.setTextColor(TFT_LIGHTGRAY, boxColor);
    lcd.setTextSize(3);
    lcd.drawString("RELEASED", x + w / 2, y + h / 2 + 10);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Wake up expanders (ensure reset lines are released)
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);
  delay(50);

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Configure PCA9536: P0 (Button 1) and P3 (Button 2) as inputs
  Wire.beginTransmission(PCA9536_ADDR);
  Wire.write(0x03); // Configuration Register
  Wire.write(0x09); // P0 (bit 0) and P3 (bit 3) as input, P1/P2 as output
  Wire.endTransmission();

  // Initialize display
  lcd.init();
  lcd.setRotation(1); // Landscape (320x240)
  lcd.fillScreen(TFT_BLACK);

  // Draw Header
  lcd.fillRect(0, 0, lcd.width(), 36, 0x0010);
  lcd.drawFastHLine(0, 36, lcd.width(), TFT_DARKCYAN);
  lcd.setTextDatum(middle_center);
  lcd.setTextColor(TFT_CYAN, 0x0010);
  lcd.setTextSize(2);
  lcd.drawString("NORVI X - BUTTON MONITOR", lcd.width() / 2, 18);

  // Initial draw
  drawButtonCard(15, 55, 140, 155, "BUTTON 1 (P0)", false);
  drawButtonCard(165, 55, 140, 155, "BUTTON 2 (P3)", false);

  Serial.println("NORVI X Button Monitor Initialized.");
}

void loop() {
  uint8_t raw = readPCA9536();
  
  if (raw != 0xFF) {
    // Buttons are active LOW (0 when pressed, 1 when released)
    bool btn1Pressed = !(raw & (1 << 0)); // P0 is Button 1
    bool btn2Pressed = !(raw & (1 << 3)); // P3 is Button 2

    // Update Button 1 card if changed
    if (btn1Pressed != prevBtn1) {
      prevBtn1 = btn1Pressed;
      drawButtonCard(15, 55, 140, 155, "BUTTON 1 (P0)", btn1Pressed);
      Serial.printf("Button 1: %s\n", btn1Pressed ? "PRESSED" : "RELEASED");
    }

    // Update Button 2 card if changed
    if (btn2Pressed != prevBtn2) {
      prevBtn2 = btn2Pressed;
      drawButtonCard(165, 55, 140, 155, "BUTTON 2 (P3)", btn2Pressed);
      Serial.printf("Button 2: %s\n", btn2Pressed ? "PRESSED" : "RELEASED");
    }
  }

  delay(50);
}
