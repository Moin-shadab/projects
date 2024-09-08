#include <LiquidCrystal.h>
#include <EEPROM.h> // To store the settings in non-volatile memory

// Pin definitions for LCD
const int LCD_RS = 8;
const int LCD_ENABLE = 9;
const int LCD_D4 = 4;
const int LCD_D5 = 5;
const int LCD_D6 = 6;
const int LCD_D7 = 7;
const int BACKLIGHT = 10;
const int BUTTON_PIN = A0; // Button connected to A0 pin

// Relay pin definitions
const int RELAY1_PIN = 2;
const int RELAY2_PIN = 3;
const int RELAY3_PIN = 4;

// Initialize the LCD with the interface pins
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Button value thresholds (based on analog values read)
int buttonValues[5] = {50, 200, 400, 600, 800}; // Approximate thresholds for Right, Up, Down, Left, Select
int currentSelection = 0; // 0: Start Machine, 1: Manual Start, 2: Settings
int topItem = 0; // Tracks the top item displayed
char* menuItems[3] = {"1: Start Machine", "2: Manual Start", "3: Settings"};
bool machineStarted = false; // Track if the machine is running
bool inManualMode = false; // Track if we're in manual mode
bool inSettingsMode = false; // Track if we're in the settings menu
int currentRelaySelection = 0; // For selecting relays in manual mode
char* relayItems[4] = {"Relay 1", "Relay 2", "Relay 3", "Back"}; // Added "Back" option

// Settings variables
int onTime = 5; // Default On Time in minutes
int offTime = 3; // Default Off Time in minutes
char* settingItems[3] = {"Set On Time", "Set Off Time", "Back"};
int currentSettingSelection = 0; // For settings menu navigation
bool settingOnTime = true; // Track whether setting On Time or Off Time

// Button press state
bool buttonPressed = false;
bool adjustTimeMode = false; // Flag for adjusting time setting
bool adjustOnTime = true; // True if adjusting On Time, false for Off Time

// Company Name
const char companyName[] = "AUMAUTOMATION ENGINEERING By MULTAN AHMED"; // Your company name
const int scrollSpeed = 200; // Adjust this value to change the scrolling speed

void setup() {
  lcd.begin(16, 2);
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH);

  // Set relay pins as output
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);

  // Initialize relays to be off
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);

  // Load settings from EEPROM
  onTime = EEPROM.read(0); // Stored On Time
  offTime = EEPROM.read(1); // Stored Off Time
  if (onTime == 255) onTime = 5; // Default value if EEPROM is empty
  if (offTime == 255) offTime = 3; // Default value if EEPROM is empty

  // Display company name with scrolling effect
  displayScrollingCompanyName();
  delay(7000); // Show company name for 5 seconds

  displayMenu(); // Show the menu
}

void loop() {
  int buttonValue = analogRead(BUTTON_PIN); // Read the analog value from A0

  if (adjustTimeMode) {
    handleAdjustTime(buttonValue); // Handle time adjustment logic
  } else if (inManualMode) {
    handleManualMode(buttonValue); // Handle manual mode logic
  } else if (inSettingsMode) {
    handleSettingsMode(buttonValue); // Handle settings mode logic
  } else {
    handleMainMenu(buttonValue); // Handle main menu logic
  }

  delay(200); // Small delay to avoid multiple presses
}

void displayScrollingCompanyName() {
  int length = strlen(companyName);
  for (int position = 0; position < length - 15; position++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(companyName + position);
    delay(scrollSpeed);
  }
}

void handleMainMenu(int buttonValue) {
  // Move the cursor based on button input
  if (buttonValue < buttonValues[1]) { // Up button pressed
    if (!machineStarted && currentSelection > 0) {
      currentSelection--;
      if (currentSelection < topItem) {
        topItem--;
      }
      displayMenu();
    }
  } else if (buttonValue < buttonValues[2]) { // Down button pressed
    if (!machineStarted && currentSelection < 2) {
      currentSelection++;
      if (currentSelection > topItem + 1) {
        topItem++;
      }
      displayMenu();
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed
    buttonPressed = true; // Mark the button as pressed

    if (!machineStarted && currentSelection == 0) { // Start Machine selected
      startMachine();
    } else if (machineStarted) { // Stop Machine selected
      stopMachine();
    } else if (!machineStarted && currentSelection == 1) { // Manual Start selected
      enterManualMode();
    } else if (!machineStarted && currentSelection == 2) { // Settings selected
      enterSettingsMode();
    }
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false; // Reset button pressed state when button is released
  }
}

void displayMenu() {
  lcd.clear();
  for (int i = 0; i < 2; i++) {
    lcd.setCursor(1, i);
    if (topItem + i < 3) {
      lcd.print(menuItems[topItem + i]);
    }
  }

  // Display the arrow to indicate current selection
  lcd.setCursor(0, currentSelection - topItem);
  lcd.write('>');
}

void startMachine() {
  machineStarted = true;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Started");
  lcd.setCursor(0, 1);
  lcd.print("Stop Machine");

  // Set relays to HIGH
  digitalWrite(RELAY1_PIN, HIGH);
  digitalWrite(RELAY2_PIN, HIGH);
  digitalWrite(RELAY3_PIN, HIGH);
}

void stopMachine() {
  machineStarted = false;
  displayMenu(); // Return to the main menu

  // Set relays to LOW (turn off)
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
}

void enterManualMode() {
  inManualMode = true;
  currentRelaySelection = 0;
  displayRelayMenu();
}

void handleManualMode(int buttonValue) {
  // Navigate through relay options
  if (buttonValue < buttonValues[1]) { // Up button pressed
    if (currentRelaySelection > 0) {
      currentRelaySelection--;
      displayRelayMenu();
    }
  } else if (buttonValue < buttonValues[2]) { // Down button pressed
    if (currentRelaySelection < 3) { // Includes "Back" option at index 3
      currentRelaySelection++;
      displayRelayMenu();
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed
    buttonPressed = true; // Mark the button as pressed
    
    if (currentRelaySelection == 3) { // "Back" option selected
      inManualMode = false; // Go back to main menu
      displayMenu();
    } else {
      toggleRelay(currentRelaySelection); // Toggle the selected relay
    }
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false; // Reset button pressed state when button is released
  }
}

void displayRelayMenu() {
  lcd.clear();
  for (int i = 0; i < 2; i++) {
    lcd.setCursor(1, i);
    if (currentRelaySelection + i < 4) { // Includes "Back" at index 3
      lcd.print(relayItems[currentRelaySelection + i]);
    }
  }
  // Display the arrow to indicate current selection
  lcd.setCursor(0, 0);
  lcd.write('>');
}

void toggleRelay(int relay) {
  switch (relay) {
    case 0: // Relay 1
      digitalWrite(RELAY1_PIN, !digitalRead(RELAY1_PIN)); // Toggle state
      lcd.clear();
      lcd.print(digitalRead(RELAY1_PIN) ? "R1 Started" : "R1 Stopped");
      break;
    case 1: // Relay 2
      digitalWrite(RELAY2_PIN, !digitalRead(RELAY2_PIN));
      lcd.clear();
      lcd.print(digitalRead(RELAY2_PIN) ? "R2 Started" : "R2 Stopped");
      break;
    case 2: // Relay 3
      digitalWrite(RELAY3_PIN, !digitalRead(RELAY3_PIN));
      lcd.clear();
      lcd.print(digitalRead(RELAY3_PIN) ? "R3 Started" : "R3 Stopped");
      break;
  }
  delay(1000); // Wait a second before returning to the relay menu
  displayRelayMenu();
}

void enterSettingsMode() {
  inSettingsMode = true;
  currentSettingSelection = 0; // Start with "Set On Time" selected
  displaySettingsMenu();
}

void handleSettingsMode(int buttonValue) {
  if (buttonValue < buttonValues[1]) { // Up button pressed
    if (currentSettingSelection > 0) {
      currentSettingSelection--;
      displaySettingsMenu();
    }
  } else if (buttonValue < buttonValues[2]) { // Down button pressed
    if (currentSettingSelection < 2) { // Includes "Back" option at index 2
      currentSettingSelection++;
      displaySettingsMenu();
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed
    buttonPressed = true; // Mark the button as pressed

    if (currentSettingSelection == 2) { // "Back" option selected
      inSettingsMode = false; // Go back to main menu
      displayMenu();
    } else {
      // Start adjusting time setting
      adjustOnTime = (currentSettingSelection == 0);
      adjustTimeMode = true;
      displayAdjustTime();
    }
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false; // Reset button pressed state when button is released
  }
}

void displaySettingsMenu() {
  lcd.clear();
  for (int i = 0; i < 2; i++) {
    lcd.setCursor(1, i);
    if (currentSettingSelection + i < 3) {
      lcd.print(settingItems[currentSettingSelection + i]);
    }
  }
  // Display the arrow to indicate current selection
  lcd.setCursor(0, 0);
  lcd.write('>');
}

void displayAdjustTime() {
  lcd.clear();
  lcd.print(adjustOnTime ? "Set On Time:" : "Set Off Time:");
  lcd.setCursor(0, 1);
  lcd.print(adjustOnTime ? onTime : offTime);
}

void handleAdjustTime(int buttonValue) {
  if (buttonValue < buttonValues[1]) { // Up button pressed (increase time)
    if (adjustOnTime) {
      onTime++;
      EEPROM.write(0, onTime);
    } else {
      offTime++;
      EEPROM.write(1, offTime);
    }
    displayAdjustTime();
  } else if (buttonValue < buttonValues[2]) { // Down button pressed (decrease time)
    if (adjustOnTime) {
      if (onTime > 0) onTime--;
      EEPROM.write(0, onTime);
    } else {
      if (offTime > 0) offTime--;
      EEPROM.write(1, offTime);
    }
    displayAdjustTime();
  } else if (buttonValue < buttonValues[4] && !buttonPressed) { // Select button pressed (save time)
    buttonPressed = true;
    adjustTimeMode = false; // Exit adjust time mode
    displaySettingsMenu();
  }

  if (buttonValue >= buttonValues[4]) {
    buttonPressed = false; // Reset button pressed state when button is released
  }
}
