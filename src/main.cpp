#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <Metro.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>

// Global things
String printname;
String inputSerial2 = "";       // a string to hold incoming data
boolean IsReadySerial2 = false; // whether the string is complete
Metro timerFlush = Metro(500);  // a timer to write to sd
Metro displayUp = Metro(1000);  // a timer to not spam the display
File logger;                    // a var to actually write to said sd
#define SCREEN_WIDTH 128        // OLED display width, in pixels
#define SCREEN_HEIGHT 64        // OLED display height, in pixels
#define OLED_RESET -1           // Reset pin # use -1 if unsure
#define SCREEN_ADDRESS 0x3c     // See datasheet for Address

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void drawThing(String msg, String pos);      // Draw something idk
float degreeToDecimal(float num, byte sign); // conversion function
String parseGll(String msg);                 // Parse GLL string function
String parseRmc(String msg);                 // Parse RMC string function
String parseGga(String msg);                 // Prase GGA string im tired
float altitudeGlobal; // Some fucking global thing because I hate parsing

// Run once on startup
void setup() {
  // Wait for Serial to start
  Serial.begin(9600);
  // while (!Serial) {
  // }

  // Wait for display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.display(); // You must call .display() after draw command to apply

  // Wait for GPS UART to start
  Serial.println("Init GPS");
  Serial2.begin(9600);
  while (!Serial2) {
  }
  // // Set GPS UART rate
  // Serial2.println("PMTK251,115200*1F");
  // delay(100);
  // Serial2.end();
  // delay(100);
  // Serial2.begin(115200);
  // while (!Serial2) {
  // }
  // page 12 of https://cdn-shop.adafruit.com/datasheets/PMTK_A11.pdf
  // checksum generator https://nmeachecksum.eqth.net/
  // you can set a value from 0 (disable) to 5 (output once every 5 pos fixes)
  // 0  NMEA_SEN_GLL,  // GPGLL interval - Lat & long
  // 1  NMEA_SEN_RMC,  // GPRMC interval - Recommended Minimum Specific GNSS
  // 2  NMEA_SEN_VTG,  // GPVTG interval - Course over Ground and Ground Speed
  // 3  NMEA_SEN_GGA,  // GPGGA interval - GPS Fix Data
  // 4  NMEA_SEN_GSA,  // GPGSA interval - GNSS DOPS and Active Satellites
  // 5  NMEA_SEN_GSV,  // GPGSV interval - GNSS Satellites in View
  // 6-17           ,  // Reserved
  // 18 NMEA_SEN_MCHN, // PMTKCHN interval – GPS channel status
  Serial2.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28");
  // Set pos fix rate to 5hz
  Serial2.println("$PMTK300,200,0,0,0,0*2F");
  // Set NEMA rate to 10hz
  Serial2.println("$PMTK220,100*2F");
  Serial.println("GPS set");

  // Wait for SD stuffs
  delay(500);
  if (!SD.begin(BUILTIN_SDCARD)) { // Begin Arduino SD API
    Serial.println("SD card failed or not present");
  }
  delay(500);

  char filename[] = "GPS_0000.CSV";
  for (uint8_t i = 0; i < 10000; i++) {
    filename[4] = i / 1000 + '0';
    filename[5] = i / 100 % 10 + '0';
    filename[6] = i / 10 % 10 + '0';
    filename[7] = i % 10 + '0';
    if (!SD.exists(filename)) { // Create and open file
      logger = SD.open(filename, (uint8_t)O_WRITE | (uint8_t)O_CREAT);
      break;
    }
    if (i == 9999) { // Print if it cant
      Serial.println("Ran out of names, clear SD");
    }
  }

  printname = filename;

  // Debug prints
  if (logger) {
    Serial.print("Successfully opened SD file: ");
    Serial.println(filename);
  } else {
    Serial.println("Failed to open SD file");
  }

  // Print guide at top of CSV
  logger.println("latitude,longitude,altitude,speed,");
  logger.flush();

  // Do te ting
  IsReadySerial2 = true;
  Serial.println("Log start");
}

// Loops forever
void loop() {
  if (IsReadySerial2) {
    // Print GPS UART
    // Serial.print(parseRmc(inputSerial2));
    Serial.println(parseGga(inputSerial2));
    Serial.println(parseRmc(inputSerial2));
    logger.print(parseRmc(inputSerial2));

    // Update display
    if (displayUp.check()) {
      display.clearDisplay();
      drawThing(printname, "top");
      drawThing(parseGga(inputSerial2), "bot");
    }

    // Reset vars
    inputSerial2 = "";
    IsReadySerial2 = false;
  }

  // Flush if timer ticked
  if (timerFlush.check()) {
    logger.flush();
  }
}

// Run on new byte from UART 1
void serialEvent2() {
  while (Serial2.available() && IsReadySerial2 == false) {
    char nextChar = char(Serial2.read()); // Cast UART data to char

    inputSerial2 += nextChar; // Append nextChar to string

    // Break the while statement once the line ends
    if (nextChar == '\n') {
      IsReadySerial2 = true;
    }
  }
}

// Structure
// $GPGLL,(lat),(N/S),(long),(E/W),(UTC),(status),(mode)*(checksum)
// more at https://gpsd.gitlab.io/gpsd/NMEA.html
String parseGll(String msg) {

  // Check that the incoming string is GLL
  if (!strstr(msg.c_str(), "GLL")) {
    return "";
  }

  // Get length of str
  int len = strlen(msg.c_str());

  // Replace commas with end character '\0' to seperate into single strings
  for (int j = 0; j < len; j++) {
    if (msg[j] == ',' || msg[j] == '*') {
      msg[j] = '\0';
    }
  }

  // A lil working var
  int i = 0;

  // Go to string i and rip things
  // Raw lattitude in degrees
  i += strlen(&msg[i]) + 1;
  float lat = atof(&msg[i]);

  // North or South char
  i += strlen(&msg[i]) + 1;
  char NS = msg[i];

  // Raw longitude in degrees
  i += strlen(&msg[i]) + 1;
  float lon = atof(&msg[i]);

  // East or West char
  i += strlen(&msg[i]) + 1;
  char EW = msg[i];

  // UTC time
  i += strlen(&msg[i]) + 1;
  float utc = atof(&msg[i]);

  // Is data valid (A) or not (V)
  i += strlen(&msg[i]) + 1;
  char valid = msg[i];

  // FAA mode
  i += strlen(&msg[i]) + 1;
  char mode = msg[i];

  // set output string to whatever
  String output = String(degreeToDecimal(lat, NS), 10) + "," +
                  String(degreeToDecimal(lon, EW), 10) + "," + '\n';

  return output;
}

// Structure
// $GPRMC,time,status,lat,N/S,lon,E/W,Speed,degrees true,
// date,degrees,FAA mode,Nav status*checksum
// more at https://gpsd.gitlab.io/gpsd/NMEA.html
String parseRmc(String msg) {

  // Check that the incoming string is GLL
  if (!strstr(msg.c_str(), "RMC")) {
    return "";
  }

  // Get length of str
  int len = strlen(msg.c_str());

  // Replace commas with end character '\0' to seperate into single strings
  for (int j = 0; j < len; j++) {
    if (msg[j] == ',' || msg[j] == '*') {
      msg[j] = '\0';
    }
  }

  // A lil working var
  int i = 0;

  // Go to string i and rip things
  // UTC time
  i += strlen(&msg[i]) + 1;
  float utc = atof(&msg[i]);

  // Is data valid (A) or not (V)
  i += strlen(&msg[i]) + 1;
  char valid = msg[i];

  // Raw lattitude in degrees
  i += strlen(&msg[i]) + 1;
  float lat = atof(&msg[i]);

  // North or South char
  i += strlen(&msg[i]) + 1;
  char NS = msg[i];

  // Raw longitude in degrees
  i += strlen(&msg[i]) + 1;
  float lon = atof(&msg[i]);

  // East or West char
  i += strlen(&msg[i]) + 1;
  char EW = msg[i];

  // spped
  i += strlen(&msg[i]) + 1;
  float speed = atof(&msg[i]);

  // Degrees true
  i += strlen(&msg[i]) + 1;
  float dtrue = atof(&msg[i]);

  // Date in ddmmyy
  i += strlen(&msg[i]) + 1;
  char date = msg[i];

  // Degrees magnetic
  i += strlen(&msg[i]) + 1;
  float magnetic = atof(&msg[i]);

  // East or West char
  i += strlen(&msg[i]) + 1;
  char EWdegree = msg[i];

  // FAA mode
  i += strlen(&msg[i]) + 1;
  char mode = msg[i];

  // A=autonomous, D=differential, E=Estimated,
  // M=Manual input mode N=not valid, S=Simulator, V = Valid
  i += strlen(&msg[i]) + 1;
  char status = msg[i];

  // set output string to whatever
  String output = String(degreeToDecimal(lat, NS), 10) + "," +
                  String(degreeToDecimal(lon, EW), 10) + "," +
                  String(altitudeGlobal, 1) + "," + '\n';

  return output;
}

// Structure
// $GPGGA,UTC,Lat,N/S,Lon,E/W,GPS Quality,# of sats,
// Precision, Altitude,Units of Altitude,Geoidal separation,
// Unit of Geoidal separation,Age of differential,station ID*Checksum
String parseGga(String msg) {

  // Check that the incoming string is GGA
  if (!strstr(msg.c_str(), "GGA")) {
    return "";
  }

  // Get length of str
  int len = strlen(msg.c_str());

  // Replace commas with end character '\0' to seperate into single strings
  for (int j = 0; j < len; j++) {
    if (msg[j] == ',' || msg[j] == '*') {
      msg[j] = '\0';
    }
  }

  // A lil working var
  int i = 0;

  // Go to string i and rip things
  // UTC time
  i += strlen(&msg[i]) + 1;
  float utc = atof(&msg[i]);

  // Lat
  i += strlen(&msg[i]) + 1;
  float lat = atof(&msg[i]);

  // N/S
  i += strlen(&msg[i]) + 1;
  char NS = msg[i];

  // Lon
  i += strlen(&msg[i]) + 1;
  float lon = atof(&msg[i]);

  // E/W
  i += strlen(&msg[i]) + 1;
  char EW = msg[i];

  // GPS quality
  // 0 - fix not available,
  // 1 - GPS fix,
  // 2 - Differential GPS fix(values above 2 are 2.3 features)
  // 3 = PPS fix
  // 4 = Real Time Kinematic
  // 5 = Float RTK
  // 6 = estimated(dead reckoning)
  // 7 = Manual input mode
  // 8 = Simulation mode
  i += strlen(&msg[i]) + 1;
  int quality = atof(&msg[i]);

  // Sats locked
  i += strlen(&msg[i]) + 1;
  int locked = atof(&msg[i]);

  // precision
  i += strlen(&msg[i]) + 1;
  float precision = atof(&msg[i]);

  // altitude
  i += strlen(&msg[i]) + 1;
  float altitude = atof(&msg[i]);
  altitudeGlobal = altitude;

  // altitude unit
  i += strlen(&msg[i]) + 1;
  char altitudeChar = msg[i];

  // The vertical distance between the surface of the
  // Earth and the surface of a model of the Earth
  // Geoidal separation
  i += strlen(&msg[i]) + 1;
  float gSep = atof(&msg[i]);

  // Geoidal separation unit
  i += strlen(&msg[i]) + 1;
  char gSepChar = msg[i];

  // Age of differential GPS data in seconds
  i += strlen(&msg[i]) + 1;
  float age = atof(&msg[i]);

  // Station ID
  i += strlen(&msg[i]) + 1;
  int station = atof(&msg[i]);

  // set output string to whatever
  // String output = String(degreeToDecimal(lat, NS), 7) + "," +
  //                 String(degreeToDecimal(lon, EW), 7) + "," + String(quality)
  //                 +
  //                 "," + String(locked) + "," + '\n';

  String output = "Q:" + String(quality) + "  #:" + String(locked);

  return output;
}

// Want to convert DDMM.MMMM to a decimal number DD.DDDDD? Slap it into this.
float degreeToDecimal(float num, byte sign) {

  int intpart = (int)num;
  float decpart = num - intpart;

  int degree = (int)(intpart / 100);
  int mins = (int)(intpart % 100);

  if (sign == 'N' || sign == 'E') {
    // Return positive degree
    return (degree + (mins + decpart) / 60);
  } else {
    // Return negative degree
    return -(degree + (mins + decpart) / 60);
  }
}

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
