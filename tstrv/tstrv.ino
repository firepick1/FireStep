// FireStep RAMPS 1.4 x-axis test revolutions (tstrv)
//
// A simple stepper test for calibrating stepper driver trimpots
// FireStep JSON: {"tstrv":[-3]}
//
// (c) 2015 Karl Lew, FirePick Services LLC

#define DELAY500NS        {asm("nop");asm("nop");asm("nop");asm("nop");       asm("nop");asm("nop");asm("nop");asm("nop");}

// TEST PARAMETERS
int16_t pinStep = 54; // X_STEP_PIN
int16_t pinDir = 55; // X_DIR_PIN
int16_t pinEnable = 38; // X_ENABLE_PIN
int32_t revs = 3; // number of revolutions before reversing direction
int32_t msRev = 500; // target speed (milliseconds/revolution)
int32_t microSteps = 16; // stepper driver microsteps
int32_t revSteps = 400; // 200:1.8degree, 400:0.9degree

// Derived variables
int16_t dir = 0;
int32_t revMicrosteps = revSteps * microSteps; // microsteps per revolution
int32_t usDelay = (msRev * 1000) / revMicrosteps; // target speed pulse delay

void setup() {
  pinMode(pinStep, OUTPUT);
  pinMode(pinDir, OUTPUT);
  pinMode(pinEnable, OUTPUT);
  digitalWrite(pinEnable, LOW);
}

void loop() {
  for (int i = 0; i < revs * revMicrosteps; i++) {
    digitalWrite(pinDir, dir);
    digitalWrite(pinStep, HIGH);
    digitalWrite(pinStep, LOW);

    delayMicroseconds(usDelay);
  }
  dir = !dir;

  delay(250);
}
