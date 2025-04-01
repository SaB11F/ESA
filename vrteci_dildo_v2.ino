#include <SoftwareSerial.h>

// Definicije pinov za motorje na CNC Shieldu
#define STEP_PIN_X 2   // X.STEP
#define DIR_PIN_X 5    // X.DIR
#define STEP_PIN_Y 3   // Y.STEP
#define DIR_PIN_Y 6    // Y.DIR

#define ENABLE_PIN 8   // Skupni ENABLE pin za A4988

// Joystick
#define VRX A0  // Levo/Desno
#define VRY A1  // Gor/Dol

// Hitrost stepper motorjev
int SMS = 800;

// APC220 inicializacija
#define APC220_RX 12
#define APC220_TX 13
SoftwareSerial apc220(APC220_RX, APC220_TX);

const uint8_t START_BYTE = 0xAA;
const size_t DATA_SIZE = 77;  // Velikost sprejetih podatkov
uint8_t buffer[128];
size_t bufferIndex = 0;

void setup() {
    Serial.begin(115200);
    apc220.begin(9600);

    // Nastavitve pinov za stepper motorje
    pinMode(STEP_PIN_X, OUTPUT);
    pinMode(DIR_PIN_X, OUTPUT);
    pinMode(STEP_PIN_Y, OUTPUT);
    pinMode(DIR_PIN_Y, OUTPUT);
    pinMode(ENABLE_PIN, OUTPUT);

    // Joystick pini
    pinMode(VRX, INPUT);
    pinMode(VRY, INPUT);

    // Omogočimo stepper driverje (LOW = vklopljen)
    digitalWrite(ENABLE_PIN, HIGH);
}

void loop() {
    //sprejemnik();
    joystickControl();
}

void sprejemnik() {
    while (apc220.available() > 0) {
        uint8_t byte = apc220.read();
        if (bufferIndex == 0 && byte != START_BYTE) {
            continue;
        }
        buffer[bufferIndex++] = byte;
        if (bufferIndex == DATA_SIZE + 2) {  // Start + 77 data + checksum
            uint8_t checksum = 0;
            for (size_t i = 1; i < DATA_SIZE + 1; i++) {
                checksum += buffer[i];
            }
            if (checksum == buffer[DATA_SIZE + 1]) {
                Serial.write(&buffer[1], DATA_SIZE);
            } else {
                Serial.println("Checksum error");
            }
            bufferIndex = 0;
        }
    }
}

void joystickControl() {
    int vrx_data = analogRead(VRX);
    int vry_data = analogRead(VRY);

    if (vrx_data > 470 && vrx_data < 530 && vry_data > 470 && vry_data < 530) {
        digitalWrite(ENABLE_PIN, HIGH); // Izklopi driver
        return;
    }

    digitalWrite(ENABLE_PIN, LOW); // Vklopi driver

    if (vrx_data > 700) moveStepper(STEP_PIN_Y, HIGH); // Desno
    if (vrx_data < 300) moveStepper(STEP_PIN_Y, LOW);  // Levo
    if (vry_data > 700) moveStepper(STEP_PIN_X, LOW);  // Dol
    if (vry_data < 300) moveStepper(STEP_PIN_X, HIGH); // Gor
}

void moveStepper(int stepPin, bool dir) {
    digitalWrite(stepPin == STEP_PIN_X ? DIR_PIN_X : DIR_PIN_Y, dir);
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(SMS);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(SMS);
}
