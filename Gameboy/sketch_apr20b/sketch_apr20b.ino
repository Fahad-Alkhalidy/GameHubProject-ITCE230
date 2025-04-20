#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Display pins
#define TFT_CS   10
#define TFT_DC   8
#define TFT_RST  9

// Joystick pins
#define JOY_UP   2
#define JOY_DWN  3
#define JOY_LFT  4
#define JOY_RHT  5
#define JOY_MID  6
#define JOY_SET  7
#define JOY_RST  12

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
