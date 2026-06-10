#include <SPI.h>
#include <SdFat.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Initialize SD Card and LCD Display
SdFat SD;
File myFile;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int chipSelect = 4; // SD Card CS Pin
const int buzzerPin = 5;  // Buzzer Pin
const int switchPin = 2;  // Switch Pin (Top = Run, Middle = Pause)

// Calibration Thresholds
const int UPPER_THRESHOLD = 235; 
const int LOWER_THRESHOLD = 215; 

void setup() {
  Serial.begin(9600);
  
  // Configure the buzzer and switch pins
  pinMode(buzzerPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP); // Uses internal pullup resistor
  
  // Initialize LCD Screen
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("System Booting...");
  
  // Quick test beep at startup
  tone(buzzerPin, 2000, 150); 
  delay(1000);

  // Initialize SD Card
  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("SD Card initialization failed!");
    lcd.clear();
    lcd.print("SD Init Failed");
    while (1); 
  }
  Serial.println("SdFat successfully initialized.");
  
  lcd.clear();
  lcd.print("Tracker Ready");
  delay(1000);
}

void loop() {
  // Read the 3-pin switch position
  // Middle Pin (Disconnected) = HIGH = Pause Mode
  // Top Pin (Connected to GND) = LOW = Run Mode
  bool isPaused = (digitalRead(switchPin) == HIGH);

  // --- CONDITION 1: SYSTEM IS PAUSED VIA SWITCH ---
  if (isPaused) {
    noTone(buzzerPin); // Keep the buzzer completely quiet
    
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("SYSTEM PAUSED   ");
    lcd.setCursor(0, 1);
    lcd.print("Switch is OFF   ");
    
    delay(200);
    return; // Skip the posture tracking while paused
  }

  // --- CONDITION 2: NORMAL OPERATION (UNPAUSED) ---
  // 1. Read Raw Flex Sensor Values
  int flexUpperData = analogRead(A0);
  int flexLowerData = analogRead(A1);

  Serial.print("Upper: "); Serial.print(flexUpperData);
  Serial.print(" | Lower: "); Serial.println(flexLowerData);

  // 2. Check Posture Logic
  bool upperSlouch = (flexUpperData < UPPER_THRESHOLD);
  bool lowerSlouch = (flexLowerData < LOWER_THRESHOLD && flexLowerData > 10); 

  // 3. Update LCD Screen, Sound Buzzer, and Log Data
  if (upperSlouch || lowerSlouch) {
    // Sound the buzzer alert
    tone(buzzerPin, 2500); 

    // Display Alert on LCD
    lcd.setCursor(0, 0);
    lcd.print("POSTURE ALERT!  ");
    lcd.setCursor(0, 1);
    if (upperSlouch && lowerSlouch) lcd.print("Full Back Slouch");
    else if (upperSlouch)           lcd.print("Upper Back Bend ");
    else                            lcd.print("Lower Back Bend ");

    // Log to SD
    myFile = SD.open("posture.txt", FILE_WRITE);
    if (myFile) {
      myFile.print("Log Entry - ");
      if (upperSlouch) myFile.print("[UPPER SLOUCH] ");
      if (lowerSlouch) myFile.print("[LOWER SLOUCH] ");
      myFile.print("Upper Raw: "); myFile.print(flexUpperData);
      myFile.print(" | Lower Raw: "); myFile.println(flexLowerData);
      myFile.close();
    }
  } else {
    // Posture is perfect! Turn off buzzer and clear screen
    noTone(buzzerPin); 
    
    lcd.setCursor(0, 0);
    lcd.print("Posture: GOOD   ");
    lcd.setCursor(0, 1);
    lcd.print("Keep it up!     ");
  }

  delay(500); 
}