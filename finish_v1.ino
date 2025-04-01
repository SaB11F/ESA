#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Adafruit_BME680.h>
#include <SD.h>

#define DHTPIN 8
#define DHTTYPE DHT11
#define SEALEVELPRESSURE_HPA (1013.25)

#define GPS_RX 7
#define GPS_TX 6
#define APC220_RX 14
#define APC220_TX 15

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BNO055 bno(55, 0x28, &Wire);
Adafruit_BME680 bme(&Wire);
SoftwareSerial ss(GPS_RX, GPS_TX);
SoftwareSerial apc220(APC220_TX, APC220_RX);
TinyGPSPlus gps;

bool headerWritten = false;

typedef struct __attribute__((packed)) {
  uint8_t id;            // 1 byte
  float bmeTemperature;  // 4 bytes
  float bmePressure;     // 4 bytes
  uint32_t bmeHumidity;  // 4 bytes
  float bmeGas;          // 4 bytes
  float altitude;        // 4 bytes
  float dhtTemperature;  // 4 bytes
  uint32_t dhtHumidity;  // 4 bytes
  float accelX;          // 4 bytes
  float accelY;          // 4 bytes
  float accelZ;          // 4 bytes
  float gyroX;           // 4 bytes
  float gyroY;           // 4 bytes
  float gyroZ;           // 4 bytes
  float magX;            // 4 bytes
  float magY;            // 4 bytes
  float magZ;            // 4 bytes
  float latitude;        // 4 bytes
  float longitude;       // 4 bytes
  uint32_t timestamp;    // 4 bytes
} str_data;              // Total: 77 bytes

str_data data;
const uint8_t START_BYTE = 0xAA;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting setup..."));
  Serial.print(F("sizeof(str_data): "));
  Serial.println(sizeof(str_data));  // Should be 77

  Wire.begin();
  /*if (!bme.begin(0x77)) {
    Serial.println(F("BME680 failed"));
  }
  if (!bno.begin()) {
    Serial.println(F("BNO055 failed"));
  }*/

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println(F("SD card initialization failed!"));
  } else {
    Serial.println(F("SD card initialized."));
  }

  ss.begin(4800);
  apc220.begin(9600);
  dht.begin();

  bme.setTemperatureOversampling(BME680_OS_2X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setGasHeater(320, 150);

  data.id = 1;
  Serial.println(F("Setup complete"));
}

void readGPS() {
  while (ss.available() > 0) {
    if (gps.encode(ss.read())) {
      if (gps.location.isValid()) {
        data.latitude = gps.location.lat();
        data.longitude = gps.location.lng();
      }
      if (gps.time.isValid()) {
        data.timestamp = gps.time.hour() * 3600UL + gps.time.minute() * 60UL + gps.time.second();
      }
    }
  }
}

void readSensors() {
  data.dhtTemperature = dht.readTemperature();
  data.dhtHumidity = dht.readHumidity();

  if (bme.performReading()) {
    data.bmeTemperature = bme.temperature;
    data.bmePressure = bme.pressure / 100.0;
    data.bmeHumidity = bme.humidity;
    data.bmeGas = bme.gas_resistance / 1000.0;
    data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  }

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
  Serial.print(F("BME Gas: "));
  Serial.print(data.bmeGas, 1);
  Serial.println(F(" KOhms"));
  Serial.print(F("Altitude: "));
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
  Serial.println();
}

void sendCompressedData() {
  apc220.write(START_BYTE);
  apc220.write((byte*)&data, sizeof(data));
  uint8_t checksum = 0;
  byte* ptr = (byte*)&data;
  for (size_t i = 0; i < sizeof(data); i++) {
    checksum += ptr[i];
  }
  apc220.write(checksum);
  apc220.flush();
  delay(200);
}

void saveData() {
  File dataFile = SD.open("2024-2025/data.csv", FILE_WRITE);
  if (dataFile) {
    // Write header row only once
    if (!headerWritten) {
      dataFile.println("ID,BME_T,BME_P,BME_H,BME_G,Alt,DHT_T,DHT_H,AccX,AccY,AccZ,GyrX,GyrY,GyrZ,MagX,MagY,MagZ,Lat,Lon,Time");
      headerWritten = true;
    }

    // Buffer for data row
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
             "%u,%.2f,%.1f,%lu,%.1f,%.1f,%.2f,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%lu",
             data.id, 
             (double)data.bmeTemperature, 
             (double)data.bmePressure, 
             data.bmeHumidity, 
             (double)data.bmeGas,
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
             data.timestamp);

    dataFile.println(buffer);
    dataFile.close();
  } else {
    Serial.println(F("Error opening data.csv"));
  }
}

void loop() {
  readSensors();
  data.id++;
  printData();
  saveData();
  sendCompressedData();
  delay(500);
}