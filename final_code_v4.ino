/*
  Version 1. - merjenje vseh senzorjev + pošiljanje prek apc220
  Version 2. - dodajanje funkcije za motorčke/nogice in shranjevanje na build in sd card
  Version 3. - čistopis kode, vse funkcije dokončane in izpopovnjene
  Version 4. - pošiljanje signala iz ground stationa na CanSat po katerem se odprejo nogice

  Made by: slogiker and SaB11F 
  Vse pravice pridržane
*/

//knjižnice
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>
//#include <Adafruit_BME680.h>
#include <BME280I2C.h>
#include <SD.h>

//definicija dht pinov
#define DHTPIN 2          // DHT11 data pin
#define DHTTYPE DHT11
#define SEALEVELPRESSURE_HPA (1013.25)

////////MOTRJI///////

//definicija pinov za motorje 
  #define M1_CW 3
  #define M1_CCW 4
  #define M2_CW 5
  #define M2_CCW 6
  #define M3_CW 7
  #define M3_CCW 8

//counter za odpiranje nogic
  static int counter = 0;


//GPS stuff
#define GPS_SERIAL Serial4  // Hardware Serial4 for GPS (pins 16 RX, 17 TX)

static const double CUSTOM_LAT = 51.508131;  // Example: London
static const double CUSTOM_LON = -0.128002;

//pini
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BNO055 bno(55, 0x28, &Wire);
//Adafruit_BME680 bme(&Wire);
SoftwareSerial ss(A1, A0);    // GPS RX, TX
SoftwareSerial apc220(15, 14);  // apc rx, tx (15,14) 
TinyGPSPlus gps;

BME280I2C bme; //bme deklariran

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

//SD header

bool headerWritten = false;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting setup..."));

  //GPS Serial
  GPS_SERIAL.begin(9600);

  Wire.begin();
  /*if (!bme.begin(0x77)) {
    Serial.println(F("BME680 failed"));
    while (1);
  }*/
  
  while (!bme.begin()) {
    Serial.println(F("Could not find BME280 sensor!"));
    delay(1000);
  }

  if (!bno.begin()) {
    Serial.println(F("BNO055 failed"));
    while (1);
  }

  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println(F("SD card failed"));
    while (1);
  }

  apc220.begin(9600); // APC220 na 9600 baudu
  dht.begin();

  /*
  bme.setTemperatureOversampling(BME680_OS_2X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setGasHeater(320, 150);
  */

  data.id = 1; // Prvi id v strukturi podatkov

  //inicializacija motorjev v setupu
  pinMode(M1_CW, OUTPUT);
  pinMode(M1_CCW, OUTPUT);
  pinMode(M2_CW, OUTPUT);
  pinMode(M2_CCW, OUTPUT);
  pinMode(M3_CW, OUTPUT);
  pinMode(M3_CCW, OUTPUT);


  Serial.println(F("Setup complete"));
}

//senzorji - GPS, BNO055, DHT, BME
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (GPS_SERIAL.available()) {
      gps.encode(GPS_SERIAL.read());
    }
  } while (millis() - start < ms);
}

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

/*
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
}*/

void senzor_BME280() {
  float temp(NAN), hum(NAN), pres(NAN); // Declare variables with NaN as default
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius); // Temperature in Celsius
  BME280::PresUnit presUnit(BME280::PresUnit_hPa);    // Pressure in hPa
  
  // Correct order: pressure, temperature, humidity
  bme.read(pres, temp, hum, tempUnit, presUnit);
  
  // Assign values to the data structure correctly
  data.bmeTemperature = temp;     // Temperature in °C
  data.bmePressure = pres;        // Pressure in hPa
  data.bmeHumidity = (uint32_t)hum; // Humidity as integer percentage
  data.altitude = 44330.0 * (1.0 - pow(data.bmePressure / SEALEVELPRESSURE_HPA, 0.1903)); // Altitude in meters
  
  // Print the corrected data
  Serial.print(F("BME Temp: ")); Serial.print(data.bmeTemperature, 2); Serial.println(F(" °C"));
  Serial.print(F("BME Pressure: ")); Serial.print(data.bmePressure, 1); Serial.println(F(" hPa"));
  Serial.print(F("BME Humidity: ")); Serial.print(data.bmeHumidity); Serial.println(F(" %"));
  Serial.print(F("BME Altitude: ")); Serial.print(data.altitude, 1); Serial.println(F(" m"));
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
             "%u,%.2f,%.1f,%lu,%.1f,%.2f,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%lu",
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
             data.timestamp);

    dataFile.println(buffer);
    dataFile.close();
  } else {
    Serial.println(F("Error opening data.csv"));
  }
}

//sending data - APC220
void sendCompressedData() {
  apc220.write(START_BYTE);              // 1 byte
  apc220.write((byte*)&data, sizeof(data));  // 81 bytes
  uint8_t checksum = 0;
  byte* ptr = (byte*)&data;
  for (size_t i = 0; i < sizeof(data); i++) {
    checksum += ptr[i];                         // Checksum over 81 bytes
  }
  apc220.write(checksum);                // 1 byte
  apc220.flush();
  delay(200);
}

void landingProtocol(){
  //kličemo landing protocol znotraj podprograma preverjanjePristanka(),
  //poravnati moramo CanSat glede na gyroscope, da bo raven
  //imamo x,y in z os glede na katere ravnamo CanSat s pomočjo 3 motorčkov, ki ga dvigujejo

  Serial.println("Pristanek zaznan! Odpiram...");
  motorForward();  // Funkcija za odpiranje
  delay(30000); // Počakamo 30 sekund, da se odprejo
  stopAllMotors(); // Ustavimo motorje po odpiranju
  Serial.println("Odpiranje končano.");
}

//premikanje motorjev

void motorForward() {
  // Vsi motorji se vrtijo v isto smer
  digitalWrite(M1_CW, HIGH);
  digitalWrite(M1_CCW, LOW);
  digitalWrite(M2_CW, HIGH);
  digitalWrite(M2_CCW, LOW);
  digitalWrite(M3_CW, HIGH);
  digitalWrite(M3_CCW, LOW);
}

void motorBackward() {
  // Vsi motorji se vrtijo v isto smer
  digitalWrite(M1_CW, LOW);
  digitalWrite(M1_CCW, HIGH);
  digitalWrite(M2_CW, LOW);
  digitalWrite(M2_CCW, HIGH);
  digitalWrite(M3_CW, LOW);
  digitalWrite(M3_CCW, HIGH);
}

void stopAllMotors() {
  // Ustavitev vseh motorjev
  digitalWrite(M1_CW, LOW);
  digitalWrite(M1_CCW, LOW);
  digitalWrite(M2_CW, LOW);
  digitalWrite(M2_CCW, LOW);
  digitalWrite(M3_CW, LOW);
  digitalWrite(M3_CCW, LOW);
}

//landing protocol - based na glasovalnem sistemu
/*void preverjanjePristanka() {
    //najprej moram dobiti altitude
    //rezerviramo trenutni altitude
    //shranimo tudi prejšnji latitude oldAltitude
    //old altitude bo enak kot zdajšnji
    //nato pa morama narediti funkcijo, ki bo preverila, če sta prejšnja in zdajšnj enaka oz. da ena ni večja od druge 

    //dodati moram counter, ki se bo vsako sekundo povečal, dokler sta stara in nova enaki, če več nista se poenostavi na 0, ko pa je counter = 5 pomeni, da je preteklo 5 sekund
    //zato lahko sprožim funkcijo kjer se CanSat poravna
    static float oldAltitude = -1;
    float currentAltitude = data.altitude / 10.0;
    float threshold = 1.0; // Prag stabilnosti višine v metrih

    if (oldAltitude < 0) { 
        oldAltitude = currentAltitude; // Inicializacija ob prvem klicu
        return;
    }

    if (abs(currentAltitude - oldAltitude) < threshold) {
        counter++;
        if (counter == 10) { // 10 sekund stabilne višine pomeni pristanek
            Serial.println("Je slo");
            landingProtocol();
        }
    } else {
        counter = 0; // Če se višina spremeni, resetiramo števec
        Serial.println("ni slo");
    }

    oldAltitude = currentAltitude;
}*/

// Globalne spremenljivke
static bool altitudeReached = false; // Spremenljivka, ki označuje, da je višina dosegla 500m
static unsigned long timeAtAltitude = 0; // Čas, ko smo dosegli 500m
static const float targetAltitude = 500.0; // Ciljna nadmorska višina v metrih
static const unsigned long timeThreshold = 10000; // Časovna omejitev 10 sekund (v ms)

// Funkcija za preverjanje višine in začetek postopka pristanka
void preverjanjeVisine() {
    float currentAltitude = data.altitude / 10.0; // trenutna nadmorska višina v metrih

    // Preveri, ali je višina dosegla 500 m prvič
    if (!altitudeReached && currentAltitude > targetAltitude) {
        // Če še nismo dosegli cilja, nastavi čas in spremenljivko
        timeAtAltitude = millis();
        altitudeReached = true; // Zabeležimo, da smo presegli 500 m
    }

    // Ko smo enkrat dosegli višino 500 m, začnemo preverjati pristajanje
    if (altitudeReached) {
        // Preveri, ali je minilo 10 sekund stabilne višine nad 500 m
        if (millis() - timeAtAltitude >= timeThreshold) {
            // Začni preverjanje pristanka, ker je bil dosežen prag 500 m vsaj 10 sekund
            preverjanjePristanka();
        }
    }
}

void preverjanjePristanka() {
    static float oldAltitude = -1;
    float currentAltitude = data.altitude / 10.0;
    float threshold = 1.0;

    Serial.print("Current Altitude: "); Serial.println(currentAltitude);
    Serial.print("Old Altitude: "); Serial.println(oldAltitude);
    Serial.print("Counter: "); Serial.println(counter);

    if (oldAltitude < 0) { 
        oldAltitude = currentAltitude;
        return;
    }

    if (abs(currentAltitude - oldAltitude) < threshold) {
        counter++;
        Serial.println("Altitude stable, counter increased.");
        if (counter >= 10) {  // Popravljeno iz "== 10" v ">= 10"
            Serial.println("Je slo, aktiviram landingProtocol()");
            landingProtocol();
        }
    } else {
        counter = 0;
        Serial.println("Ni slo, counter reset.");
    }

    oldAltitude = currentAltitude;
}


//iz ground stationa pošiljamo signal, da se naj nogice razprejo. Ko se ta signal izvrši kličemo funkcijo ki razpre nogice.
void receiveCommand() {
    if (apc220.available() > 0) {
        uint8_t command = apc220.read();
        if (command == 0xF0) {
            Serial.println("Prejet ukaz za odpiranje nogic!");
            //openLegs(); // Klic funkcije za odpiranje nogic
        }
    }
}

//Debugg
void readSensors() {
  Serial.print(F("ID: ")); Serial.println(data.id);
  senzor_DHT();
  //senzor_BME();
  senzor_BNO055();
  readGPS();
  data.id++;
  senzor_BME280();
}

void loop() {
  readSensors();
  saveData();  

  // Simulacija: višina se ne spreminja (pristanek)
  static float testAltitude = 5100.0; // Simulirana začetna višina
  data.altitude = testAltitude * 10; // Pretvori v cm (ker v kodi množiš *10)


  Serial.print("Višina: ");
  Serial.print(data.altitude / 10.0); // Prikaz dejanske višine
  Serial.print(" m | Števec stabilnosti: ");
  Serial.println(counter); // Prikazuje števec

  preverjanjeVisine();  

  //sendCompressedData();
  //receiveCommand();
  //motorForward();
  delay(1000);
}
