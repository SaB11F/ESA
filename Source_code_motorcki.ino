// Definicija pinov za DRV8833
const int motor1A = 3;
const int motor1B = 4;
const int motor2A = 5;
const int motor2B = 6;
const int motor3A = 7;
const int motor3B = 8;


int counter = 0;

void setup() {
  // Nastavitev pinov kot izhodne
  pinMode(motor1A, OUTPUT);
  pinMode(motor1B, OUTPUT);
  pinMode(motor2A, OUTPUT);
  pinMode(motor2B, OUTPUT);
  pinMode(motor3A, OUTPUT);
  pinMode(motor3B, OUTPUT);

  // Začetno stanje - ustavimo motorje
  stopAllMotors();
}

void loop() {
  // Vrtimo vse motorje v isto smer (npr. naprej)
  motorForward();
  delay(32000); // Vrti motorje 5 sekund

  // Ustavimo motorje
  stopAllMotors();
  delay(10000); // Počakaj 2 sekundi

  motorBackward();
  delay(32000);

  stopAllMotors();
  delay(10000);

  counter++;
  Serial.println(counter);
}

void motorForward() {
  // Vsi motorji se vrtijo v isto smer
  digitalWrite(motor1A, HIGH);
  digitalWrite(motor1B, LOW);
  digitalWrite(motor2A, HIGH);
  digitalWrite(motor2B, LOW);
  digitalWrite(motor3A, HIGH);
  digitalWrite(motor3B, LOW);
}

void motorBackward() {
  // Vsi motorji se vrtijo v isto smer
  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, HIGH);
  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, HIGH);
  digitalWrite(motor3A, LOW);
  digitalWrite(motor3B, HIGH);
}

void stopAllMotors() {
  // Ustavitev vseh motorjev
  digitalWrite(motor1A, LOW);
  digitalWrite(motor1B, LOW);
  digitalWrite(motor2A, LOW);
  digitalWrite(motor2B, LOW);
  digitalWrite(motor3A, LOW);
  digitalWrite(motor3B, LOW);
}