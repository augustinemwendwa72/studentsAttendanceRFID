#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

/* ---------- LCD ---------- */
#define LCD_CONTRAST 6
LiquidCrystal lcd(7, 5, 4, 3, 2, A0);
SoftwareSerial serialIn(8, 9);  // RX, TX - receives from ESP8266
SoftwareSerial gsm(10, 11);     // RX, TX - SIM800L GSM module


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

/* ---------- GSM SMS FUNCTIONS ---------- */
void sendSMS(String phone, String studentName, String cardNo) {
    Serial.println("Starting GSM...");
    
    // Begin GSM serial only when needed
    gsm.begin(9600);
    delay(1000);
    
    Serial.println("Sending SMS...");
    lcdStatus("Sending SMS...", "");
    
    // Wait for GSM to initialize
    delay(2000);
    
    // Start AT command
    gsm.println("AT");
    delay(1000);
    printGSMResponse();
    
    // Set SMS to text mode
    gsm.println("AT+CMGF=1");
    delay(1000);
    printGSMResponse();
    
    // Start SMS to phone number
    gsm.print("AT+CMGS=\"");
    gsm.print(phone);
    gsm.println("\"");
    delay(1000);
    printGSMResponse();
    
    // Message content
    gsm.print("Dear parent, your student ");
    gsm.print(studentName);
    gsm.print(" has checked with our school using the rfid card ");
    gsm.print(cardNo);
    gsm.println(" just Now.");
    gsm.println("Thank you.");
    gsm.println();
    gsm.print("Regards, Shikondi Girls School");
    
    // Send Ctrl+Z to transmit
    gsm.write(26);
    delay(3000);
    printGSMResponse();
    
    Serial.println("SMS Sent!");
    lcdStatus("SMS Sent!", "");
    delay(2000);
    
    // Stop GSM serial to free pins for serialIn
    gsm.end();
    delay(1000);
    serialIn.begin(9600);
    Serial.println("GSM stopped, serialIn available");
    lcdStatus("Shikondi Girls", "System Ready");
}

void printGSMResponse() {
    while (gsm.available()) {
        Serial.write(gsm.read());
    }
}


// ------------------- SETUP -------------------
void setup() {
    serialIn.begin(9600);   // For ESP8266 communication
    // gsm.begin(9600);     // Don't begin GSM in setup - it will interfere with serialIn
    Serial.begin(9600);
  lcd.begin(16, 2);

  pinMode(LCD_CONTRAST, OUTPUT);
  analogWrite(LCD_CONTRAST, 100);
  Serial.println("Exiting Setup");
  lcdStatus("Shikondi Girls", "Attendance SYSTM");
}

void loop() {
  // put your main code here, to run repeatedly:
  while(serialIn.available()){
    String data = serialIn.readStringUntil('\n');
    Serial.println("====================");
    Serial.println(data);
    Serial.println("====================");
    // Parse the incoming data
    parseData(data);
    if(data.indexOf("Wifi not found") != 0){
        lcdStatus("Shikondi Girls", "WIFI Not Found");
        Serial.println("WIFI NF");
        delay(2000);
    }
    if(data.indexOf("Connecting to: shikondi")!= 0){
        lcdStatus("Connecting to: ", "shikondi Wifi");
        Serial.println("Conn 2 Shkd");
        delay(2000);
    }
    // Display on LCD based on status and message
    if (status == "false" && message.length() == 0) {
        // Empty message - show "Swipe Again" and RFID
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Swipe Again");
        lcd.setCursor(0, 1);
        lcd.print(rfidID);
        
        // lcdStatus("Swipe Again!", rfidID);
    } else if (status == "false" && message == "Student not found") {
        // Student not found - show RFID and "NOT FOUND"
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(rfidID);
        lcd.setCursor(0, 1);
        lcd.print("NOT FOUND");
        // lcdStatus(rfidID, "NOT FOUND");
    } else {
        // Otherwise (true or with message) - show RFID + Name on line 1, Phone on line 2
        if(name.length() > 0)lcd.clear();
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
        delay(3000);
    }
    
    // Send SMS if phone number is found and status is true
    if (status == "true" && phoneNumber.length() > 0) {
        sendSMS(phoneNumber, name, rfidID);
    }
    
    Serial.println(data);
  }
}
