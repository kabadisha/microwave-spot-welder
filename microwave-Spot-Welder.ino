#include <Bounce2.h> // https://github.com/thomasfredericks/Bounce2
#include <B20M04_4x8.h> // https://github.com/kabadisha/B20M04_4x8_Segment_Display

// Create an instance of the display with pins:
// 10 as SLAVESELECT
// 11 as MOSI
// 13 as CLOCK
B20M04_4x8 disp(5, 6, 7);

const int POT_PIN = A0; // The pin connected to the pulseMs selection potentiometer
const int FIRE_RELAY_PIN = 9; // The pin connected to the firing relay
const int FIRE_BUTTON_PIN = 4;

// Instantiate a Bounce2 Button object for debouncing switch input
Bounce2::Button FIRE_BUTTON = Bounce2::Button();

const int MAX_PULSE_MS = 10000;
const int PULSE_INCREMENT_MS = 250;

static unsigned char FIRE_STATE = LOW;

int selectedPulseMs = PULSE_INCREMENT_MS;

// Define the number of samples to keep track of. The higher the number, the
// more the readings will be smoothed, but the slower the output will respond to
// the input. Using a constant rather than a normal variable lets us use this
// value to determine the size of the readings array.
const int NUM_POT_READINGS = 30;

int POT_READINGS[NUM_POT_READINGS]; // the readings from the potentiometer input
int POT_READ_INDEX = 0;             // the index of the current reading
int POT_READINGS_TOTAL = 0;         // the running total

void setup() {
  // Turn on the built-in LED to show power is on.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Serial.println("Booting...");
  disp.displayText("Boot.");
  
  pinMode(FIRE_RELAY_PIN, OUTPUT);
  FIRE_BUTTON.attach(FIRE_BUTTON_PIN, INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  FIRE_BUTTON.interval(25); // Use a debounce interval of 25 milliseconds
  FIRE_BUTTON.setPressedState(LOW); // INDICATE THAT THE LOW STATE CORRESPONDS TO PHYSICALLY PRESSING THE BUTTON
  delay(1000);

  // Initialize all the pot readings array.
  // This array is used in getSelectedPulseMs() in order to smooth readings from the potentiometer.
  for (int thisReading = 0; thisReading < NUM_POT_READINGS; thisReading++) {
    int value = analogRead(POT_PIN);
    POT_READINGS[thisReading] = value;
    POT_READINGS_TOTAL += value;
    delay(1); // delay in between reads for stability
  }
  
  disp.displayText(".");
  delay(500);
  disp.displayText("..");
  delay(500);
  disp.displayText("...");
  delay(500);
  disp.displayText("....");
  delay(500);
  disp.displayText("On");
  delay(500);
  disp.displayInteger(selectedPulseMs);
  Serial.print("selectedPulseMs: ");
  Serial.println(selectedPulseMs);
}

void loop() {
  // Only update the desired pulse ms if the welder is not currently active.
  if (FIRE_STATE == LOW) {
    
    // Read the input value from the pulseMs selection potentiometer:
    int pulseLengthPotValue = getSelectedPulseMs();
  
    // We devide the pot input into 20 'steps'
    int potentiometerIncrements = MAX_PULSE_MS/PULSE_INCREMENT_MS;
    int potentiometerInput = constrain(pulseLengthPotValue / (1024/potentiometerIncrements), 1, potentiometerIncrements);
  
    int newPulseMs = potentiometerInput * PULSE_INCREMENT_MS;
    
    // Debugging
    //Serial.print("pulseLengthPotValue: ");
    //Serial.println(pulseLengthPotValue);
  
    updateDisplay(newPulseMs);
  }

  FIRE_BUTTON.update(); // Update the Bounce instance

  // For safety, any time the trigger button is not CURRENTLY depressed, the welding head is disabled.
  // This way, the user has to hold the trigger to get the full duration.
  // Releasing the trigger will always disable the welding head.
  if (FIRE_STATE == HIGH && !FIRE_BUTTON.isPressed()) {
    digitalWrite(FIRE_RELAY_PIN, LOW);
    FIRE_STATE = LOW;
    Serial.println("Trigger released. Welding head disabled.");
    disp.displayInteger(selectedPulseMs);
  }

  // If the welding head has been on for more than the selected pulse increment then turn it off.
  if(FIRE_STATE == HIGH && FIRE_BUTTON.currentDuration() > selectedPulseMs) {
    digitalWrite(FIRE_RELAY_PIN, LOW);
    FIRE_STATE = LOW;
    Serial.println("Welding head finished firing.");
    disp.displayInteger(selectedPulseMs);
  }

  // If the trigger button has been pressed since last loop, then turn fire the welding head.
  if (FIRE_BUTTON.pressed() && FIRE_STATE == LOW) {
    Serial.print("Firing welding head for: ");
    Serial.print(selectedPulseMs);
    Serial.println("ms");
    disp.displayText("FIRE");
    FIRE_STATE = HIGH;
    digitalWrite(FIRE_RELAY_PIN, HIGH);
  }
}

void updateDisplay(int _value) {
  if (_value != selectedPulseMs) {
    selectedPulseMs = _value;
    disp.displayInteger(selectedPulseMs);
    Serial.print("selectedPulseMs: ");
    Serial.println(selectedPulseMs);
  }
}

/*
 * This function smooths values from the potentiometer by returning the average value of
 * a number of previous readings. The number of readings used is defined by NUM_POT_READINGS.
 */
int getSelectedPulseMs() {
  // subtract the oldest reading:
  POT_READINGS_TOTAL = POT_READINGS_TOTAL - POT_READINGS[POT_READ_INDEX];
  // read from the sensor and store it in the oldest array slot.
  POT_READINGS[POT_READ_INDEX] = analogRead(POT_PIN);
  delay(1); // delay in between reads for stability
  // add the reading to the total:
  POT_READINGS_TOTAL = POT_READINGS_TOTAL + POT_READINGS[POT_READ_INDEX];
  // advance to the next position in the array:
  POT_READ_INDEX = POT_READ_INDEX + 1;

  // if we're at the end of the array...
  if (POT_READ_INDEX >= NUM_POT_READINGS) {
    // ...wrap around to the beginning:
    POT_READ_INDEX = 0;
  }

  /* Debugging
  Serial.print("Total: ");
  Serial.print(POT_READINGS_TOTAL);
  Serial.print(" Array: ");
  for (int i = 0; i < NUM_POT_READINGS; i++) {
    Serial.print(POT_READINGS[i]);
    Serial.print("+");
  }
  Serial.println();
  */
  // calculate the average:
  return POT_READINGS_TOTAL / NUM_POT_READINGS;
}
