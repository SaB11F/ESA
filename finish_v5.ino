/*
  Version 1. - Merjenje vseh senzorjev in preverjanje njihovega delovanja
  Version 2. - Pošiljanje prek modula apc220 in shranjevanje na SD
  Version 3. - Dodajanje kode za nogice in logike za pristajanje
  Version 4. - Združevanje kode za nogice in branje/pošiljanje senzorjev
  Version 5. - Čistopis kode in testiranje celega delovanja

  Made by: slogiker and SaB11F 
  Vse pravice pridržane
*/



#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <DHT.h>
#include <TinyGPSPlus.h>
#include <BME280I2C.h>
#include <SD.h>

// Pin and constant definitions
#define DHTPIN 2
#define DHTTYPE DHT11
#define SEALEVELPRESSURE_HPA (1013.25)
#define GPS_SERIAL Serial4  // Hardware Serial4 for GPS (pins 16 RX, 17 TX)
#define APC220_SERIAL Serial3  // APC220 on Serial3

// Motor pin definitions
#define M1_CW 3
#define M1_CCW 4
#define M2_CW 5
#define M2_CCW 6
#define M3_CW 7
#define M3_CCW 8

static const double CUSTOM_LAT = 51.508131;  // Example: London
static const double CUSTOM_LON = -0.128002;

// Sensor objects
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BNO055 bno(55, 0x28, &Wire);
BME280I2C bme;
TinyGPSPlus gps;

bool headerWritten = false;

// Data structure
typedef struct __attribute__((packed)) {
  uint8_t id;              // 1 byte
  float bmeTemperature;    // 4 bytes
  float bmePressure;       // 4 bytes
  uint32_t bmeHumidity;    // 4 bytes
  float altitude;          // 4 bytes
  float dhtTemperature;    // 4 bytes
  uint32_t dhtHumidity;    // 4 bytes
  float accelX;            // 4 bytes
  float accelY;            // 4 bytes
  float accelZ;            // 4 bytes
  float gyroX;             // 4 bytes
  float gyroY;             // 4 bytes
  float gyroZ;             // 4 bytes
  float magX;              // 4 bytes
  float magY;              // 4 bytes
  float magZ;              // 4 bytes
  float latitude;          // 4 bytes
  float longitude;         // 4 bytes
  uint32_t timestamp;      // 4 bytes
  float gpsAltitude;       // 4 bytes
  float distanceToCustom;  // 4 bytes
} str_data;  // Total: 81 bytes

str_data data;
const uint8_t START_BYTE = 0xAA;

// Motor control variables
static bool altitudeReached = false;
static unsigned long timeAtAltitude = 0;
static const float targetAltitude = 500.0;  // Target altitude in meters
static const unsigned long timeThreshold = 10000;  // 10 seconds in ms
static int counter = 0;
static bool isLanding = false;
static unsigned long motorStartTime = 0;

void setup() {
  //Serial.begin(115200);
  /*Serial.println(F("Starting setup..."));
  Serial.print(F("sizeof(str_data): "));
  Serial.println(sizeof(str_data));  // Confirms 81 bytes*/

  Wire.begin();

  while (!bme.begin()) {
   // Serial.println(F("Could not find BME280 sensor!"));
    delay(1000);
  }
  if (!bno.begin()) {
   // Serial.println(F("BNO055 failed"));
  }
  if (!SD.begin(BUILTIN_SDCARD)) {
    //Serial.println(F("SD card initialization failed!"));
  }

  GPS_SERIAL.begin(9600);
  APC220_SERIAL.begin(9600);
  dht.begin();

  // Initialize motor pins
  pinMode(M1_CW, OUTPUT);
  pinMode(M1_CCW, OUTPUT);
  pinMode(M2_CW, OUTPUT);
  pinMode(M2_CCW, OUTPUT);
  pinMode(M3_CW, OUTPUT);
  pinMode(M3_CCW, OUTPUT);

  data.id = 1;
  //Serial.println(F("Setup complete"));
}

// Motor control functions
void motorForward() {
  digitalWrite(M1_CW, HIGH);
  digitalWrite(M1_CCW, LOW);
  digitalWrite(M2_CW, HIGH);
  digitalWrite(M2_CCW, LOW);
  digitalWrite(M3_CW, HIGH);
  digitalWrite(M3_CCW, LOW);
}

void motorBackward() {
  digitalWrite(M1_CW, LOW);
  digitalWrite(M1_CCW, HIGH);
  digitalWrite(M2_CW, LOW);
  digitalWrite(M2_CCW, HIGH);
  digitalWrite(M3_CW, LOW);
  digitalWrite(M3_CCW, HIGH);
}

void stopAllMotors() {
  digitalWrite(M1_CW, LOW);
  digitalWrite(M1_CCW, LOW);
  digitalWrite(M2_CW, LOW);
  digitalWrite(M2_CCW, LOW);
  digitalWrite(M3_CW, LOW);
  digitalWrite(M3_CCW, LOW);
}

// Sensor reading functions
void readGPS() {
  if (gps.location.isValid()) {
    data.latitude = gps.location.lat();
    data.longitude = gps.location.lng();
    data.distanceToCustom = TinyGPSPlus::distanceBetween(
      gps.location.lat(), gps.location.lng(), CUSTOM_LAT, CUSTOM_LON) / 1000.0;  // km
  }
  if (gps.time.isValid()) {
    data.timestamp = gps.time.hour() * 3600UL + gps.time.minute() * 60UL + gps.time.second();
  }
  if (gps.altitude.isValid()) {
    data.gpsAltitude = gps.altitude.meters();
  }
}

void readSensors() {
  data.dhtTemperature = dht.readTemperature();
  data.dhtHumidity = dht.readHumidity();

  float temp(NAN), hum(NAN), pres(NAN);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_hPa);
  bme.read(pres, temp, hum, tempUnit, presUnit);

  data.bmeTemperature = temp;
  data.bmePressure = pres;
  data.bmeHumidity = (uint32_t)hum;
  data.altitude = 44330.0 * (1.0 - pow(data.bmePressure / SEALEVELPRESSURE_HPA, 0.1903));

  sensors_event_t accelData, gyroData, magData;
  bno.getEvent(&accelData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&gyroData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno.getEvent(&magData, Adafruit_BNO055::VECTOR_MAGNETOMETER);
  data.accelX = accelData.acceleration.x;
  data.accelY = accelData.acceleration.y;
  data.accelZ = accelData.acceleration.z;
  data.gyroX = gyroData.gyro.x;
  data.gyroY = gyroData.gyro.y;
  data.gyroZ = gyroData.gyro.z;
  data.magX = magData.magnetic.x;
  data.magY = magData.magnetic.y;
  data.magZ = magData.magnetic.z;

  readGPS();
}

void printData() {
  Serial.print(F("ID: "));
  Serial.println(data.id);
  Serial.print(F("BME Temp: "));
  Serial.print(data.bmeTemperature, 2);
  Serial.println(F(" °C"));
  Serial.print(F("BME Pressure: "));
  Serial.print(data.bmePressure, 1);
  Serial.println(F(" hPa"));
  Serial.print(F("BME Humidity: "));
  Serial.print(data.bmeHumidity);
  Serial.println(F(" %"));
  Serial.print(F("BME Altitude: "));
  Serial.print(data.altitude, 1);
  Serial.println(F(" m"));
  Serial.print(F("DHT Temp: "));
  Serial.print(data.dhtTemperature, 2);
  Serial.println(F(" °C"));
  Serial.print(F("DHT Humidity: "));
  Serial.print(data.dhtHumidity);
  Serial.println(F(" %"));
  Serial.print(F("Accel X: "));
  Serial.print(data.accelX, 2);
  Serial.println(F(" m/s²"));
  Serial.print(F("Accel Y: "));
  Serial.print(data.accelY, 2);
  Serial.println(F(" m/s²"));
  Serial.print(F("Accel Z: "));
  Serial.print(data.accelZ, 2);
  Serial.println(F(" m/s²"));
  Serial.print(F("Gyro X: "));
  Serial.print(data.gyroX, 2);
  Serial.println(F(" deg/s"));
  Serial.print(F("Gyro Y: "));
  Serial.print(data.gyroY, 2);
  Serial.println(F(" deg/s"));
  Serial.print(F("Gyro Z: "));
  Serial.print(data.gyroZ, 2);
  Serial.println(F(" deg/s"));
  Serial.print(F("Mag X: "));
  Serial.print(data.magX, 2);
  Serial.println(F(" uT"));
  Serial.print(F("Mag Y: "));
  Serial.print(data.magY, 2);
  Serial.println(F(" uT"));
  Serial.print(F("Mag Z: "));
  Serial.print(data.magZ, 2);
  Serial.println(F(" uT"));
  Serial.print(F("Latitude: "));
  Serial.print(data.latitude, 6);
  Serial.println(F(" deg"));
  Serial.print(F("Longitude: "));
  Serial.print(data.longitude, 6);
  Serial.println(F(" deg"));
  Serial.print(F("Timestamp: "));
  Serial.println(data.timestamp);
  Serial.print(F("GPS Altitude: "));
  Serial.print(data.gpsAltitude, 2);
  Serial.println(F(" m"));
  Serial.print(F("Distance to Custom: "));
  Serial.print(data.distanceToCustom, 2);
  Serial.println(F(" km"));
  Serial.print(F("Satellites: "));
  Serial.println(gps.satellites.value());
  Serial.println();
}

// Data handling functions
void sendCompressedData() {
  APC220_SERIAL.write(START_BYTE);              // 1 byte
  APC220_SERIAL.write((byte*)&data, sizeof(data));  // 81 bytes
  uint8_t checksum = 0;
  byte* ptr = (byte*)&data;
  for (size_t i = 0; i < sizeof(data); i++) {
    checksum += ptr[i];                         // Checksum over 81 bytes
  }
  APC220_SERIAL.write(checksum);                // 1 byte
  APC220_SERIAL.flush();
  delay(200);
}

void saveData() {
  File dataFile = SD.open("2024-2025/data.csv", FILE_WRITE);
  if (dataFile) {
    if (!headerWritten) {
      dataFile.println("ID,BME_T,BME_P,BME_H,Alt,DHT_T,DHT_H,AccX,AccY,AccZ,GyrX,GyrY,GyrZ,MagX,MagY,MagZ,Lat,Lon,Time,GPS_Alt,Dist_Custom");
      headerWritten = true;
    }

    char buffer[300];
    snprintf(buffer, sizeof(buffer), 
             "%u,%.2f,%.1f,%lu,%.1f,%.2f,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%lu,%.2f,%.2f",
             data.id, 
             (double)data.bmeTemperature, 
             (double)data.bmePressure, 
             data.bmeHumidity, 
             (double)data.altitude, 
             (double)data.dhtTemperature, 
             data.dhtHumidity,
             (double)data.accelX, 
             (double)data.accelY, 
             (double)data.accelZ,
             (double)data.gyroX, 
             (double)data.gyroY, 
             (double)data.gyroZ,
             (double)data.magX, 
             (double)data.magY, 
             (double)data.magZ,
             (double)data.latitude, 
             (double)data.longitude, 
             data.timestamp,
             (double)data.gpsAltitude,
             (double)data.distanceToCustom);

    dataFile.println(buffer);
    dataFile.close();
  } else {
    //Serial.println(F("Error opening data.csv"));
  }
}

// Motor control logic
void preverjanjeVisine() {
  float currentAltitude = data.altitude;  // Altitude in meters
  if (!altitudeReached && currentAltitude > targetAltitude) {
    timeAtAltitude = millis();
    altitudeReached = true;
  }
  if (altitudeReached && (millis() - timeAtAltitude >= timeThreshold)) {
    preverjanjePristanka();
  }
}

void preverjanjePristanka() {
  static float oldAltitude = -1;
  float currentAltitude = data.altitude;
  float threshold = 1.0;  // Stability threshold in meters

  if (oldAltitude < 0) {
    oldAltitude = currentAltitude;
    return;
  }

  if (abs(currentAltitude - oldAltitude) < threshold) {
    counter++;

    if (counter >= 10 && !isLanding) {  // 10 stable readings (~5s)
    //Serial.println("startaj");
      landingProtocol();
    }
  } else {
    counter = 0;
  }
  oldAltitude = currentAltitude;
}

void landingProtocol() {
  if (!isLanding) {
    //Serial.println("Pristanek zaznan! Odpiram...");
    motorForward();
    motorStartTime = millis();
    isLanding = true;
  }
}

void checkMotorTimeout() {
  if (isLanding && (millis() - motorStartTime >= 30000)) {  // 30 seconds
    stopAllMotors();
    //Serial.println("Odpiranje končano.");
    isLanding = false;
  }
}

void loop() {
  smartDelay(100);  // Process GPS for 100ms
  readSensors();
  data.id++;
  printData();  
  saveData();
  sendCompressedData();
  preverjanjeVisine();
  checkMotorTimeout();
  delay(400);  // ~500ms total loop time
}

static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (GPS_SERIAL.available()) {
      gps.encode(GPS_SERIAL.read());
    }
  } while (millis() - start < ms);
}