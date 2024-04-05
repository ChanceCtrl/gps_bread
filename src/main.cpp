#include <Arduino.h>
#include <Metro.h>
#include <SD.h>

// Global things
Metro timerFlush = Metro(500); // a timer to write to sd
File track;                    // a var to actually write to said sd
File points;                   // Another file class guy to store points

// Button handeler to re-use later
#include "pol.hpp"
polButton theguy;

// Vector Nav things
#include "nav.hpp"
vNav nd;

// Run once on startup
void setup() {
  // Wait for Serial to start
  Serial.begin(9600);
  // Get vector nav boi ready
  nd.init_NAV();
  // Setup button for point logging
  pinMode(14, INPUT_PULLUP);
  theguy.init(14, 500.0, digitalRead, true);

  // Wait for SD stuffs
  delay(500);
  if (!SD.begin(BUILTIN_SDCARD)) { // Begin Arduino SD API
    Serial.println("SD card failed or not present");
  }
  delay(500);

  // TODO: Make this not look braindead
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
}

// Loops forever
void loop() {
  // If we have more than 4 new bytes, see if its a new line
  if (Serial8.available() > 4) {
    if (nd.check_sync_byte()) {
      // Get NAV data
      nd.read_nav_data();

      track.print(String(nd.lat_lon[0], 10) + ",");
      track.println(String(nd.lat_lon[1], 10));
    }
  }

  if (theguy.state == true && theguy.held == false) {
    points.print(String(nd.lat_lon[0], 10) + ",");
    points.println(String(nd.lat_lon[1], 10));
  }

  // Flush if timer ticked
  if (timerFlush.check()) {
    track.flush();
    points.flush();
  }

  // Update button state
  theguy.update();
}
