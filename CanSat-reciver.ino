#include <SoftwareSerial.h>

#define APC220_RX 3
#define APC220_TX 2
SoftwareSerial apc220(APC220_RX, APC220_TX);

const uint8_t START_BYTE = 0xAA;
const size_t DATA_SIZE = 77;  // Size of str_data
uint8_t buffer[128];          // Small buffer: start + data + checksum = 79 bytes
size_t bufferIndex = 0;

void setup() {
    Serial.begin(115200);
    apc220.begin(9600);
    //Serial.println("Ground station ready...");
}

void loop() {
    while (apc220.available() > 0) {
    uint8_t byte = apc220.read();
    if (bufferIndex == 0 && byte != START_BYTE) {
        continue;  // Skip until start byte
    }
    buffer[bufferIndex++] = byte;
    if (bufferIndex == DATA_SIZE + 2) {  // Start + 77 data + checksum
        uint8_t checksum = 0;
        for (size_t i = 1; i < DATA_SIZE + 1; i++) {
            checksum += buffer[i];
        }
        if (checksum == buffer[DATA_SIZE + 1]) {
            Serial.write(&buffer[1], DATA_SIZE);  // Send only the 77-byte data
        }
        // Remove or comment out: Serial.println("Checksum error");
        bufferIndex = 0;  // Reset for next packet
    }
}
}
