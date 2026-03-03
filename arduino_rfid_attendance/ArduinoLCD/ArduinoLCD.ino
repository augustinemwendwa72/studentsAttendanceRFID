#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

/* ---------- LCD ---------- */
#define LCD_CONTRAST 6
LiquidCrystal lcd(7, 5, 4, 3, 2, A0);
SoftwareSerial serialIn(8, 9);  // RX, TX


/* ---------- VARIABLES FOR PARSED DATA ---------- */
String rfidID = "";
String status = "";
String message = "";
String phoneNumber = "";
String name = "";

/* ---------- PARSE INCOMING DATA ---------- */
void parseData(String data) {
    // Reset variables
    rfidID = "";
    status = "";
    message = "";
    phoneNumber = "";
    name = "";
    
    // Find first pipe (RFID ends here)
    int firstPipe = data.indexOf('|');
    if (firstPipe > 0) {
        rfidID = data.substring(0, firstPipe);
        
        // Find second pipe (status ends here)
        int secondPipe = data.indexOf('|', firstPipe + 1);
        if (secondPipe > firstPipe) {
            status = data.substring(firstPipe + 1, secondPipe);
            // Remove leading colon if present (handle ":true" or ":false")
            if (status.startsWith(":")) {
                status = status.substring(1);
            }
            
            // Find third pipe (message ends here)
            int thirdPipe = data.indexOf('|', secondPipe + 1);
            if (thirdPipe > secondPipe) {
                message = data.substring(secondPipe + 1, thirdPipe);
                
                // Find fourth pipe (phone ends here)
                int fourthPipe = data.indexOf('|', thirdPipe + 1);
                if (fourthPipe > thirdPipe && fourthPipe < data.length() - 1) {
                    phoneNumber = data.substring(thirdPipe + 1, fourthPipe);
                    
                    // Everything after fourth pipe is the name
                    if (fourthPipe + 1 < data.length()) {
                        name = data.substring(fourthPipe + 1);
                        // Remove trailing pipe if present
                        if (name.endsWith("|")) {
                            name = name.substring(0, name.length() - 1);
                        }
                    }
                } else if (thirdPipe + 1 < data.length()) {
                    // No fourth pipe - might be just message and status
                    phoneNumber = data.substring(thirdPipe + 1);
                }
            }
        }
    }
    
    // Print parsed values to Serial for debugging
    Serial.println("--- Parsed Data ---");
    Serial.print("RFID: "); Serial.println(rfidID);
    Serial.print("Status: "); Serial.println(status);
    Serial.print("Message: "); Serial.println(message);
    Serial.print("Phone: "); Serial.println(phoneNumber);
    Serial.print("Name: "); Serial.println(name);

    Serial.println("-------------------");
}

/* ---------- LCD HELPERS ---------- */
void lcdStatus(const char *l1, const char *l2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  lcd.print(l2);
}


// ------------------- SETUP -------------------
void setup() {
    serialIn.begin(9600);
    Serial.begin(9600);
  lcd.begin(16, 2);

  pinMode(LCD_CONTRAST, OUTPUT);
  analogWrite(LCD_CONTRAST, 100);
}

void loop() {
  // put your main code here, to run repeatedly:
  while(serialIn.available()){
    String data = serialIn.readStringUntil('\n');
    
    // Parse the incoming data
    parseData(data);
    
    // Display on LCD based on status and message
    if (status == "false" && message.length() == 0) {
        // Empty message - show "Swipe Again" and RFID
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Swipe Again");
        lcd.setCursor(0, 1);
        lcd.print(rfidID);
    } else if (status == "false" && message == "Student not found") {
        // Student not found - show RFID and "NOT FOUND"
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(rfidID);
        lcd.setCursor(0, 1);
        lcd.print("NOT FOUND");
    } else {
        // Otherwise (true or with message) - show RFID + Name on line 1, Phone on line 2
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(rfidID);
        if (name.length() > 0) {
            // Truncate if too long for LCD
            if (name.length() > 16) {
                lcd.print(" ");
                lcd.print(name.substring(0, 13));
            } else {
                lcd.print(" ");
                lcd.print(name);
            }
        }
        lcd.setCursor(0, 1);
        lcd.print(phoneNumber);
    }
    
    Serial.println(data);
  }
}
