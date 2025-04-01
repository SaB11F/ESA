/* Koda za en motor, ki se vrti v eno smer */
//poravnanje motorja

const int sig1 = 11;
const int sig2 = 10;

void setup() {
    pinMode(sig1, OUTPUT);
    pinMode(sig2, OUTPUT);
    digitalWrite(sig1, LOW);
    digitalWrite(sig2, LOW);
}

void loop() {
    motor_CW();
    delay(1000);
    stopMotor();
    delay(1000);
}

void stopMotor() {
    digitalWrite(sig1, LOW);
    digitalWrite(sig2, LOW);
}

void motor_CW() {
    digitalWrite(sig1, HIGH);
    digitalWrite(sig2, LOW);
}
