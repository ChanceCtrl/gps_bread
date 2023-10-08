#include <Arduino.h>
#include <Metro.h>
#include <SD.h>

// Global things
String inputSerial1 = ""; // a string to hold incoming data
String loggerString = "";
boolean IsReadySerial1 = false; // whether the string is complete
Metro timer_flush = Metro(500); // a timer to write to sd
File logger;                    // a var to actually write to said sd
float degreeToDecimal(float num, byte sign); // conversion function
String parseGll(String msg);                 // Parse GLL string function
String parseRmc(String msg);                 // Parse RMC string function

// Run once on startup
void setup() {
  // Wait for Serial to start
  Serial.begin(9600);
  while (!Serial) {
  }

  // Wait for GPS UART to start
  Serial.println("Init GPS");
  Serial1.begin(9600);
  while (!Serial1) {
  }

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
  Serial1.println("$PMTK314,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29");
  // Set update loop to 10hz
  Serial1.println("$PMTK220,100*2F");
  Serial.println("GPS set");

  // Wait for SD stuffs
  delay(500);
  if (!SD.begin(BUILTIN_SDCARD)) { // Begin Arduino SD API (Teensy 3.5)
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

  // Debug prints
  if (logger) {
    Serial.print("Successfully opened SD file: ");
    Serial.println(filename);
  } else {
    Serial.println("Failed to open SD file");
  }

  // Print guide at top of CSV
  logger.println("latitude,longitude,");
  logger.flush();

  // Do te ting
  IsReadySerial1 = true;
  Serial.println("Log start");
}

// Loops forever
void loop() {
  if (IsReadySerial1) {
    // Print GPS UART
    Serial.println(parseGll(inputSerial1));
    logger.println(parseGll(inputSerial1));

    // Append result to a string to not send too many writes to SD
    // loggerString += parseGll(inputSerial1) + '\n';
    // Serial.print(loggerString);

    // Flush if timer ticked
    if (timer_flush.check()) {
      logger.flush();
    }

    // Reset vars
    inputSerial1 = "";
    IsReadySerial1 = false;
  }
}

// Run on new byte from UART 1
void serialEvent1() {
  while (Serial1.available() && IsReadySerial1 == false) {
    char nextChar = char(Serial1.read()); // Cast UART data to char

    inputSerial1 += nextChar; // Append nextChar to string

    // Break the while statement once the line ends
    if (nextChar == '\n') {
      IsReadySerial1 = true;
    }
  }
}

// Structure
// $GPGLL,(lat),(N/S),(long),(E/W),(UTC),(status),(mode)*(checksum)
// more at https://gpsd.gitlab.io/gpsd/NMEA.html
String parseGll(String msg) {

  // Check that the incoming string is GLL
  if (!strstr(msg.c_str(), "GLL")) {
    return "Ya shit wack, go check your GPS setup for GLL";
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
  float lat_raw = atof(&msg[i]);

  // North or South char
  i += strlen(&msg[i]) + 1;
  char latNS = msg[i];

  // Raw longitude in degrees
  i += strlen(&msg[i]) + 1;
  float lon_raw = atof(&msg[i]);

  // East or West char
  i += strlen(&msg[i]) + 1;
  char lonEW = msg[i];

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
  String output = String(degreeToDecimal(lat_raw, latNS), 7) + "," +
                  String(degreeToDecimal(lon_raw, lonEW), 7) + ",";

  return output;
}

// Unfinished, RMC has a lot more data to parse
// Structure
// $GPRMC,time,status,lat,N/S,lon,E/W,Speed,degrees true,
// date,degrees,FAA mode,Nav status*checksum
// more at https://gpsd.gitlab.io/gpsd/NMEA.html
String parseRmc(String msg) {

  // Check that the incoming string is GLL
  if (!strstr(msg.c_str(), "RMC")) {
    return "Ya shit wack, go check your GPS setup for RMC";
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
  float lat_raw = atof(&msg[i]);

  // North or South char
  i += strlen(&msg[i]) + 1;
  char latNS = msg[i];

  // Raw longitude in degrees
  i += strlen(&msg[i]) + 1;
  float lon_raw = atof(&msg[i]);

  // East or West char
  i += strlen(&msg[i]) + 1;
  char lonEW = msg[i];

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
  char lonEWdegree = msg[i];

  // FAA mode
  i += strlen(&msg[i]) + 1;
  char mode = msg[i];

  // A=autonomous, D=differential, E=Estimated,
  // M=Manual input mode N=not valid, S=Simulator, V = Valid
  i += strlen(&msg[i]) + 1;
  char status = msg[i];

  // set output string to whatever
  String output = String(degreeToDecimal(lat_raw, latNS), 7) + "," +
                  String(degreeToDecimal(lon_raw, lonEW), 7) + "," +
                  String(speed, 4);

  return output;
}

float degreeToDecimal(float num, byte sign) {
  // Want to convert DDMM.MMMM to a decimal number DD.DDDDD

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
