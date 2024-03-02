#ifndef dis
#define dis

// Imports or something
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Metro.h>

Metro displayUp = Metro(1000); // a timer to not spam the display

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # use -1 if unsure
#define SCREEN_ADDRESS 0x3c // See datasheet for Address

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Some function defs
void drawThing(String msg, String pos); // Cursed draw function

// It take string in, and it puts shit onto display
void drawThing(String msg, String pos) {
  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);                 // Use full 256 char 'Code Page 437'

  if (pos == "top") {
    display.setCursor(0, 0);
    display.print(msg);
  } else if (pos == "bot") {
    display.setTextSize(2.5);
    display.setCursor(0, 32);
    display.print(msg);
  } else {
    return;
  }

  display.display();
}

#endif
