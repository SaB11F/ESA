/*
  Version 1. - merjenje vseh senzorjev + pošiljanje prek apc220
  Version 2. - dodajanje funkcije za motorčke/nogice in shranjevanje na build in sd card
  Version 3. - čistopis kode, vse funkcije dokončane in izpopovnjene

  Made by: slogiker and SaB11F 
  Vse pravice pridržane
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
#include <Adafruit_BME680.h>
#include <SD.h>

#define DHTPIN 8          // DHT11 data pin
#define DHTTYPE DHT11
#define SEALEVELPRESSURE_HPA (1013.25)

////////MOTRJI///////
#include "Cdrv8833.h"   //knjiznica za driverje

//definicija pinov za motorje 
  #define M1_CW 12
  #define M1_CCW 13
  #define M2_CW 14
  #define M2_CCW 15
  #define M3_CW 16
  #define M3_CCW 17

//inicializacija motorjev - objekt
  Cdrv8833 motor1;
  Cdrv8833 motor2;
  Cdrv8833 motor3;

//

DHT dht(DHTPIN, DHTTYPE);
Adafruit_BNO055 bno(55, 0x28, &Wire);
Adafruit_BME680 bme(&Wire);
SoftwareSerial ss(A1, A0);    // GPS RX, TX
SoftwareSerial apc220(2, 3);  // APC220 RX, TX
TinyGPSPlus gps;

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

  Wire.begin();
  if (!bme.begin(0x77)) {
    Serial.println(F("BME680 failed"));
    while (1);
  }

  if (!bno.begin()) {
    Serial.println(F("BNO055 failed"));
    while (1);
  }

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println(F("SD card failed"));
    while (1);
  }

  ss.begin(4800);    // GPS na 4800 baudu
  apc220.begin(9600); // APC220 na 9600 baudu
  dht.begin();

  bme.setTemperatureOversampling(BME680_OS_2X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setGasHeater(320, 150);

  data.id = 1; // Prvi id v strukturi podatkov

  //inicializacija motorjev v setupu
  motor1.init(M1_CW, M1_CCW, 0, false);
  motor1.init(M2_CW, M2_CCW, 1, false);
  motor1.init(M3_CW, M3_CCW, 2, false);

  Serial.println(F("Setup complete"));
}

//senzorji - GPS, BNO055, DHT, BME
void senzor_GPS() {
  while (ss.available() > 0) {
    if (gps.encode(ss.read())) {
      if (gps.location.isValid()) {
        data.latitude = gps.location.lat() * 1000000;
        data.longitude = gps.location.lng() * 1000000;
      }
      if (gps.time.isValid()) {
        data.timestamp = gps.time.hour() * 3600UL + gps.time.minute() * 60UL + gps.time.second();
      }
    }
  }
}

void senzor_BNO055() {

  sensors_event_t accelData, gyroData, magData;
  bno.getEvent(&accelData, Adafruit_BNO055::VECTOR_ACCELEROMETER);
  bno.getEvent(&gyroData, Adafruit_BNO055::VECTOR_GYROSCOPE);
  bno.getEvent(&magData, Adafruit_BNO055::VECTOR_MAGNETOMETER);

  //acceleration
  data.accelX = accelData.acceleration.x * 100;
  data.accelY = accelData.acceleration.y * 100;
  data.accelZ = accelData.acceleration.z * 100;

  //gyro 
  data.gyroX = gyroData.gyro.x * 100;
  data.gyroY = gyroData.gyro.y * 100;
  data.gyroZ = gyroData.gyro.z * 100;

  //magnitude
  data.magX = magData.magnetic.x * 100;
  data.magY = magData.magnetic.y * 100;
  data.magZ = magData.magnetic.z * 100;

  //debugg
  Serial.print(F("Accel X: ")); Serial.print(data.accelX / 100.0, 2); Serial.println(F(" m/s²"));
  Serial.print(F("Accel Y: ")); Serial.print(data.accelY / 100.0, 2); Serial.println(F(" m/s²"));
  Serial.print(F("Accel Z: ")); Serial.print(data.accelZ / 100.0, 2); Serial.println(F(" m/s²"));

  Serial.print(F("Gyro X: ")); Serial.print(data.gyroX / 100.0, 2); Serial.println(F(" deg/s"));
  Serial.print(F("Gyro Y: ")); Serial.print(data.gyroY / 100.0, 2); Serial.println(F(" deg/s"));
  Serial.print(F("Gyro Z: ")); Serial.print(data.gyroZ / 100.0, 2); Serial.println(F(" deg/s"));

  Serial.print(F("Mag X: ")); Serial.print(data.magX / 100.0, 2); Serial.println(F(" uT"));
  Serial.print(F("Mag Y: ")); Serial.print(data.magY / 100.0, 2); Serial.println(F(" uT"));
  Serial.print(F("Mag Z: ")); Serial.print(data.magZ / 100.0, 2); Serial.println(F(" uT"));

  Serial.print(F("Latitude: ")); Serial.print(data.latitude / 1000000.0, 6); Serial.println(F(" deg"));
  Serial.print(F("Longitude: ")); Serial.print(data.longitude / 1000000.0, 6); Serial.println(F(" deg"));
  Serial.print(F("Timestamp: ")); Serial.println(data.timestamp);
  Serial.println();
}

void senzor_DHT(){
    data.dhtTemperature = dht.readTemperature() * 100;
    data.dhtHumidity = dht.readHumidity();

    //debbug
    Serial.print(F("DHT Temp: ")); Serial.print(data.dhtTemperature / 100.0, 2); Serial.println(F(" °C"));
    Serial.print(F("DHT Humidity: ")); Serial.print(data.dhtHumidity); Serial.println(F(" %"));
}

void senzor_BME(){

    //if BME performing run - starts reading
    if (bme.performReading()) {
    data.bmeTemperature = bme.temperature * 100;
    data.bmePressure = (bme.pressure / 100.0) * 10;
    data.bmeHumidity = bme.humidity;
    data.bmeGas = bme.gas_resistance / 100;
    data.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA) * 10;
    }

    //writing value to serial print
    Serial.print(F("BME Temp: ")); Serial.print(data.bmeTemperature / 100.0, 2); Serial.println(F(" °C"));
    Serial.print(F("BME Pressure: ")); Serial.print(data.bmePressure / 10.0, 1); Serial.println(F(" hPa"));
    Serial.print(F("BME Humidity: ")); Serial.print(data.bmeHumidity); Serial.println(F(" %"));
    Serial.print(F("BME Gas: ")); Serial.print(data.bmeGas / 10.0, 1); Serial.println(F(" KOhms"));
    Serial.print(F("Altitude: ")); Serial.print(data.altitude / 10.0, 1); Serial.println(F(" m"));
}

//SD - shranjevanje na SD kartico
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

//sending data - APC220
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

void landingProtocol(){
  //kličemo landing protocol znotraj podprograma preverjanjePristanka(),
  //poravnati moramo CanSat glede na gyroscope, da bo raven
  //imamo x,y in z os glede na katere ravnamo CanSat s pomočjo 3 motorčkov, ki ga dvigujejo

  //vsaka izmed smeri mora biti 0, da je CanSat poravnan, preveri za ziher
  float targetY = 0.0;
  float targetX = 0.0;
  float targetZ = 9.81;  //razen Z, ker naj bi bil enak kot gravitacijski pospesek 9.81...

  //current values naših senzorjev - x,y,z
  float currentY = gyroData.gyro.x * 100;
  float currentX = gyroData.gyro.y * 100;
  float currentZ = gyroData.gyro.z * 100;

  //Funkcija za obračanje
  while(currentX != targetX || currentY != targetY || currentZ != targetZ){
    motor1.run(255);
    motor2.run(255);
    motor3.run(255);
  }
}

//landing protocol - based na glasovalnem sistemu
void preverjanjePristanka() {
    //najprej moram dobiti altitude
    //rezerviramo trenutni altitude
    //shranimo tudi prejšnji latitude oldAltitude
    //old altitude bo enak kot zdajšnji
    //nato pa morama narediti funkcijo, ki bo preverila, če sta prejšnja in zdajšnj enaka oz. da ena ni večja od druge 

    //dodati moram counter, ki se bo vsako sekundo povečal, dokler sta stara in nova enaki, če več nista se poenostavi na 0, ko pa je counter = 5 pomeni, da je preteklo 5 sekund
    //zato lahko sprožim funkcijo kjer se CanSat poravna

    static int counter = 0;

    static float oldAltitude = 0;
    float currentAltitude = data.altitude / 10.0;   //zdajšnji

    //funkcija
    if(abs(currentAltitude - oldAltitude) < 0.1){
      counter = counter + 1;    //povečamo counter za 1
      if(counter > 5){
        //ko je counter več kot 5 (5 sekund) zaženemo podprogram za pristanek
        landingProtocol();
      }
    }else{
      //čene counter poenostavimo na 0
      counter = 0;
    }

    oldAltitude = currentAltitude;
}

//Debugg
void readSensors() {
  Serial.print(F("ID: ")); Serial.println(data.id);
  senzor_DHT();
  senzor_BME();
  senzor_BNO055();
  senzor_GPS();
  data.id++;
}

void loop() {
  readSensors();
  saveData();
  sendCompressedData();
  delay(1000);
}
