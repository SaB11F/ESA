/*
  Version 1. - imamo merjenje vseh senzorjev in pošiljanje s pomočjo APC220 + Save to SD
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

typedef struct {
  uint8_t id;
  int16_t bmeTemperature;
  int16_t bmePressure;
  uint8_t bmeHumidity;
  int16_t bmeGas;
  int16_t altitude;
  int16_t dhtTemperature;
  uint8_t dhtHumidity;
  int16_t accelX, accelY, accelZ;
  int16_t gyroX, gyroY, gyroZ;
  int16_t magX, magY, magZ;
  int32_t longitude;
  int32_t latitude;
  uint32_t timestamp;
} str_data;

str_data data;

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

  ss.begin(4800);    // GPS at 4800 baud
  apc220.begin(9600); // APC220 at 9600 baud
  dht.begin();

  bme.setTemperatureOversampling(BME680_OS_2X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setGasHeater(320, 150);

  data.id = 1; // Fixed ID, adjust as needed

  /*if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println(F("SD card failed"));
    while (1);
  }*/

  File dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(F("ID,BME_Temp,BME_Pressure,BME_Humidity,BME_Gas,Altitude,DHT_Temp,DHT_Humidity,Accel_X,Accel_Y,Accel_Z,Gyro_X,Gyro_Y,Gyro_Z,Mag_X,Mag_Y,Mag_Z,Latitude,Longitude,Timestamp"));
    dataFile.close();
  }

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
void saveToSD() {
  File dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(data.id);
    dataFile.print(",");
    dataFile.print(data.bmeTemperature / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.bmePressure / 10.0, 1);
    dataFile.print(",");
    dataFile.print(data.bmeHumidity);
    dataFile.print(",");
    dataFile.print(data.bmeGas / 10.0, 1);
    dataFile.print(",");
    dataFile.print(data.altitude / 10.0, 1);
    dataFile.print(",");
    dataFile.print(data.dhtTemperature / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.dhtHumidity);
    dataFile.print(",");
    dataFile.print(data.accelX / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.accelY / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.accelZ / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.gyroX / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.gyroY / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.gyroZ / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.magX / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.magY / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.magZ / 100.0, 2);
    dataFile.print(",");
    dataFile.print(data.latitude / 1000000.0, 6);
    dataFile.print(",");
    dataFile.print(data.longitude / 1000000.0, 6);
    dataFile.print(",");
    dataFile.print(data.timestamp);
    dataFile.println();
    dataFile.close();
  } else {
    Serial.println(F("Error opening data.csv"));
  }
}

//sending data - APC220
void sendCompressedData() {
  apc220.write((byte*)&data, sizeof(data));
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
        pristanek();
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
  //saveToSD();
  sendCompressedData();
  delay(1000);
}