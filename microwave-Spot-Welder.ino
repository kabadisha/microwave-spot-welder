#include <Bounce2.h> // https://github.com/thomasfredericks/Bounce2
#include <B20M04_4x8.h> // https://github.com/kabadisha/B20M04_4x8_Segment_Display

// Create an instance of the display with pins:
// 10 as SLAVESELECT
// 11 as MOSI
// 13 as CLOCK
B20M04_4x8 disp(10, 11, 13);

const int POT_PIN = A0; // The pin connected to the pulseMs selection potentiometer
const int FIRE_RELAY_PIN = 3; // The pin connected to the firing relay
const int FIRE_BUTTON_PIN = 2;

// Instantiate a Bounce2 Button object for debouncing switch input
Bounce2::Button FIRE_BUTTON = Bounce2::Button();

const int MAX_PULSE_MS = 10000;
const int PULSE_INCREMENT_MS = 250;

static unsigned char FIRE_STATE = LOW;

int selectedPulseMs = PULSE_INCREMENT_MS;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Serial.println("Booting...");
  disp.displayText("Boot.");
  
  pinMode(FIRE_RELAY_PIN, OUTPUT);
  FIRE_BUTTON.attach(FIRE_BUTTON_PIN, INPUT_PULLUP); // Attach the debouncer to a pin with INPUT_PULLUP mode
  FIRE_BUTTON.interval(25); // Use a debounce interval of 25 milliseconds
  FIRE_BUTTON.setPressedState(LOW); // INDICATE THAT THE LOW STATE CORRESPONDS TO PHYSICALLY PRESSING THE BUTTON
  delay(1000);
  
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
}

void loop() {
  // Only update the desired pulse ms if the welder is not currently active.
  if (FIRE_STATE == LOW) {
    // Read the input value from the pulseMs selection potentiometer:
    int pulseLengthPotValue = analogRead(POT_PIN);
    delay(1); // delay in between reads for stability
  
    // We devide the pot input into 20 'steps'
    int potentiometerIncrements = MAX_PULSE_MS/PULSE_INCREMENT_MS;
    int potentiometerInput = constrain(pulseLengthPotValue / (1024/potentiometerIncrements), 1, potentiometerIncrements);
  
    int newPulseMs = potentiometerInput * PULSE_INCREMENT_MS;
    
    // Debugging
    //Serial.print("selectedPulseMs: ");
    //Serial.println(selectedPulseMs);
  
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
  }

  // If the welding head has been on for more than the selected pulse increment then turn it off.
  if(FIRE_STATE == HIGH && FIRE_BUTTON.currentDuration() > selectedPulseMs) {
    digitalWrite(FIRE_RELAY_PIN, LOW);
    FIRE_STATE = LOW;
    Serial.println("Welding head finished firing.");
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
