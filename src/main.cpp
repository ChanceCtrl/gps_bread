#include <Arduino.h>
#include <Metro.h>
#include <SD.h>

// Global things
String printname;
Metro timerFlush = Metro(500); // a timer to write to sd
File track;                    // a var to actually write to said sd
File points;                   // Another file class guy to store points

#define POL 13

// #define HAS_GPS
#define HAS_NAV
// #define HAS_DIS

#ifdef HAS_GPS
#include "gps.hpp"
#endif

#ifdef HAS_NAV
#include "nav.hpp"
navData nd;
#endif

#ifdef HAS_DIS
#include "dis.hpp"
#endif

// Run once on startup
void setup() {
  // Setup pin for point logging
  pinMode(POL, INPUT_PULLDOWN);

  // Wait for Serial to start
  Serial.begin(9600);

#ifdef HAS_GPS
  init_GPS();
#endif

#ifdef HAS_NAV
  init_NAV();
#endif

#ifdef HAS_DIS
  // Wait for display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.display(); // You must call .display() after draw command to apply
#endif

  // Wait for SD stuffs
  delay(500);
  if (!SD.begin(BUILTIN_SDCARD)) { // Begin Arduino SD API
    Serial.println("SD card failed or not present");
  }
  delay(500);

  char filename[] = "GPS_0000.CSV";
  for (uint16_t i = 0; i < 10000; i++) {
    filename[4] = i / 1000 + '0';
    filename[5] = i / 100 % 10 + '0';
    filename[6] = i / 10 % 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename)) { // Create and open file
      track = SD.open(filename, (uint8_t)O_WRITE | (uint8_t)O_CREAT);
      break;
    }
    if (i == 9999) { // Print if it cant
      Serial.println("Ran out of names, clear SD");
    }
  }

  printname = filename; // For display redraws

  char filename2[] = "PPS_0000.CSV";
  for (uint16_t i = 0; i < 10000; i++) {
    filename2[4] = i / 1000 + '0';
    filename2[5] = i / 100 % 10 + '0';
    filename2[6] = i / 10 % 10 + '0';
    filename2[7] = i % 10 + '0';
    if (!SD.exists(filename2)) { // Create and open file
      points = SD.open(filename2, (uint8_t)O_WRITE | (uint8_t)O_CREAT);
      break;
    }
    if (i == 9999) { // Print if it cant
      Serial.println("Ran out of names, clear SD");
    }
  }

  // Print guide at top of CSV
  track.println("latitude, longitude");
  track.flush();

  // Print another guide
  points.println("latitude, longitude");
  points.flush();

  // Do te ting
  Serial.println("Log start");
}

// Loops forever
void loop() {
#ifdef HAS_GPS
  if (ready_serial8) {
    // If new line is ready add to log
    track.println(parse_rmc(input_serial8));

// Display sats and lock
#ifdef HAS_DIS
    if (displayUp.check()) {
      display.clearDisplay();
      drawThing(printname, "top");
      drawThing(parse_gga(input_serial8), "bot");
    }
#endif

    // Reset vars
    ready_serial8 = false;
  }
#endif

#ifdef HAS_NAV
  // Reset nav state
  nav_ready = false;

  // If we have more than 4 new bytes, see if its a new line
  if (Serial8.available() > 4)
    check_sync_byte();

  // If check_sync_byte() set is_nav_ready true, log data
  if (nav_ready) {
    // Get NAV data
    nd = read_nav_data();

    track.print(String(nd.r_pos[0], 10) + ",");
    track.println(String(nd.r_pos[1], 10));
  }

// Display sats and lock
#ifdef HAS_DIS
  if (displayUp.check()) {
    display.clearDisplay();
    drawThing(printname, "top");
    drawThing(parse_gga(input_serial8), "bot");
  }
#endif
#endif

  // Flush if timer ticked
  if (timerFlush.check()) {
    track.flush();
    points.flush();
  }
}

#ifdef HAS_GPS
void serialEvent8() {
  // Wait till we want more data
  while (Serial8.available() && ready_serial8 == false) {
    char nextChar = char(Serial8.read()); // Cast UART data to char

    input_serial8 += nextChar; // Append nextChar to string

    // Break the while statement once the line ends
    if (nextChar == '\n') {
      ready_serial8 = true;
    }
  }
}
#endif
