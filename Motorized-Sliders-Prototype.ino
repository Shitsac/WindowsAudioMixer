const int NUM_SLIDERS = 5;
const int analogInputs[NUM_SLIDERS] = {A1, A2, A3, A4, A5};
const int ioOutputs[NUM_SLIDERS * 2] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
const int pwmOutputs[NUM_SLIDERS] = {11, 12, 13, 9, 8};  // PWM pins for motor speed control

int sliderValues[NUM_SLIDERS];
int newSliderValues[NUM_SLIDERS];
bool newValue = false;
int deadband = 20;  // Allow some tolerance to prevent jitter
int maxSpeed = 255; // Maximum PWM value (speed)

void setup() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    pinMode(analogInputs[i], INPUT);
    pinMode(pwmOutputs[i], OUTPUT);  // Initialize PWM pins for speed control
  }

  for (int i = 0; i < NUM_SLIDERS * 2; i++) {
    pinMode(ioOutputs[i], OUTPUT);  // Initialize motor direction control pins
  }

  Serial.begin(9600);
}

void loop() {
  updateSliderValues();
  sendSliderValues();
  receivedSliderValues();
  setSliderPosition();
  delay(10);
}

// Updates current value for every input
void updateSliderValues() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    sliderValues[i] = analogRead(analogInputs[i]);
  }
}

// Sends slider values via serial in the form of a string
void sendSliderValues() {
  String builtString = String("");

  for (int i = 0; i < NUM_SLIDERS; i++) {
    builtString += String((int)sliderValues[i]);

    if (i < NUM_SLIDERS - 1) {
      builtString += String("|");
    }
  }
  if (newValue == false) {
    Serial.println(builtString);
  }
}

void receivedSliderValues() {
  while (Serial.available() > 0) {
    String receivedString = Serial.readStringUntil('\n');
    char* token = strtok(const_cast<char*>(receivedString.c_str()), "|");

    for (int i = 0; token != NULL && i < NUM_SLIDERS; i++) {
      newSliderValues[i] = atoi(token);
      token = strtok(NULL, "|");
    }
  }
}

// Set the motor to adjust slider position
void setSliderPosition() {
  for (int i = 0; i < NUM_SLIDERS; i++) {
    int diff = sliderValues[i] - newSliderValues[i];  // Calculate difference
    
    // If the difference exceeds the deadband, move the motor
    if (abs(diff) > deadband) {
      newValue = true;  // Mark that a new value is being processed

      int motorSpeed = map(abs(diff), deadband, 1023, 100, maxSpeed);  // Map difference to speed
      
      // Ensure motorSpeed is within valid PWM range
      motorSpeed = constrain(motorSpeed, 0, maxSpeed);
      
      if (sliderValues[i] < newSliderValues[i]) {
        // Move motor UP
        digitalWrite(ioOutputs[i * 2], HIGH);   // Set direction
        digitalWrite(ioOutputs[i * 2 + 1], LOW);
        analogWrite(pwmOutputs[i], motorSpeed);  // Adjust speed with PWM
      } else {
        // Move motor DOWN
        digitalWrite(ioOutputs[i * 2], LOW);   // Set direction
        digitalWrite(ioOutputs[i * 2 + 1], HIGH);
        analogWrite(pwmOutputs[i], motorSpeed);  // Adjust speed with PWM
      }
    } else {
      // Motor is in position, stop it
      digitalWrite(ioOutputs[i * 2], LOW);  // Turn off motor
      digitalWrite(ioOutputs[i * 2 + 1], LOW);
      analogWrite(pwmOutputs[i], 0);  // Set speed to 0 (stop motor)

      // Send acknowledgment to C# that the motor has reached its target position
      if (newValue == true) {
        Serial.print("P");
        Serial.print(i + 1);
        Serial.println(":done");  // Format as P1:done, P2:done, etc.
        newValue = false;
      }
    }
  }
}
