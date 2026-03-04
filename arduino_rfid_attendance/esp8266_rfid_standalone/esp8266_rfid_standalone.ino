/*
 * ESP8266 RFID Attendance Scanner (Standalone)
 * 
 * This version uses ESP8266 directly with Wemos D1 R2 board
 * Connect RFID RC522 directly to ESP8266
 * 
 * Wiring:
 * RFID RST -> D1 (GPIO5)
 * RFID SDA  -> D2 (GPIO4)
 * RFID MOSI -> D7 (GPIO13)
 * RFID MISO -> D6 (GPIO12)
 * RFID SCK  -> D5 (GPIO14)
 */

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>

// ==================== PIN CONFIGURATION ====================
#define RST_PIN D1    // D1 - RFID RST
#define SS_PIN D2     // D2 - RFID SDA

// ==================== WIFI CONFIGURATION ====================
const char* ssid = "shikondi";          // Replace with your WiFi SSID
const char* password = "shikondi2026!";  // Replace with your WiFi password

// ==================== SERVER CONFIGURATION ====================
const char* serverHost = "b254systems.work.gd";    // Replace with server IP
const int serverPort = 3100;

// ==================== VARIABLES ====================
MFRC522 rfid(SS_PIN, RST_PIN);

// #define LED_BUILTIN 2      // Built-in LED (D4)
#define LED_SUCCESS 16     // D0 - Success LED
#define LED_ERROR D8        // D1 - Error LED (same as RST, adjust if needed)

String lastCardUID = "";
unsigned long lastScanTime = 0;
const unsigned long SCAN_COOLDOWN = 3000;  // 3 seconds cooldown

// Software Serial on D3 (RX) and D4 (Tx)
SoftwareSerial serialOut(D3, D4);  // RX, TX

void setup() {
    Serial.begin(9600);
    serialOut.begin(9600);  // SoftwareSerial for output
    SPI.begin();
    
    // Initialize RFID
    rfid.PCD_Init();
    Serial.println("RFID Reader initialized");
    
    // Initialize LEDs
    // pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_SUCCESS, OUTPUT);
    pinMode(LED_ERROR, OUTPUT);
    
    // Turn off LEDs
    // digitalWrite(LED_BUILTIN, HIGH);  // Built-in LED is inverted
    digitalWrite(LED_SUCCESS, LOW);
    digitalWrite(LED_ERROR, LOW);
    
    // Connect to WiFi
    connectWiFi();
    
    Serial.println("System ready!");
    Serial.println("Scan an RFID card to mark attendance...");
}

void loop() {
    // Check for new cards
    if (rfid.PICC_IsNewCardPresent()) {
        if (rfid.PICC_ReadCardSerial()) {
            // Get card UID
            String uidString = "";
            for (byte i = 0; i < rfid.uid.size; i++) {
                uidString += String(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
                uidString += String(rfid.uid.uidByte[i], HEX);
            }
            uidString.toUpperCase();
            
            // Prevent duplicate scans
            unsigned long currentTime = millis();
            if (uidString != lastCardUID || (currentTime - lastScanTime) > SCAN_COOLDOWN) {
                lastCardUID = uidString;
                lastScanTime = currentTime;
                
                Serial.print("Card detected: ");
                Serial.println(uidString);
                
                // Send attendance to server
                sendAttendance(uidString);
            }
            
            // Halt PICC
            rfid.PICC_HaltA();
        }
    }
    
    delay(100);
}

// ==================== WIFI CONNECTION ====================
void connectWiFi() {
    delay(3000);
    Serial.println("Starting WiFi connection...");
    Serial.print("SSID: ");
    Serial.println(ssid);
    serialOut.println("Connecting to: " + String(ssid));
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
        
        // Print current WiFi status
        Serial.print(" Status: ");
        Serial.println(WiFi.status());
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        // digitalWrite(LED_BUILTIN, LOW);  // Turn on built-in LED
    } else {
        Serial.println("\nWiFi connection failed!");
        Serial.print("Final WiFi Status: ");
        Serial.println(WiFi.status());
        
        // Print specific error
        switch(WiFi.status()) {
            case WL_NO_SSID_AVAIL:
                Serial.println("Error: SSID not available");
                serialOut.println("Wifi not found");
                break;
            case WL_CONNECT_FAILED:
                Serial.println("Error: Connection failed (wrong password?)");
                serialOut.println("Wrong Password-wifi");
                break;
            case WL_IDLE_STATUS:
                Serial.println("Error: WiFi idle - module not started");
                break;
            case WL_DISCONNECTED:
                Serial.println("Error: WiFi disconnected");
                serialOut.println("Wifi Disconnected");
                break;
            default:
                Serial.println("Error: Unknown error");
                break;
        }
        digitalWrite(LED_ERROR, HIGH);
    }
}

// ==================== SEND ATTENDANCE TO SERVER ====================
void sendAttendance(String rfidCard) {
    Serial.println("Connecting to server...");
    serialOut.println("Sending to server");
    delay(1000);
    
    WiFiClient client;
    
    if (client.connect(serverHost, serverPort)) {
        Serial.println("Connected to server!");
        
        // Build HTTP GET request
        String url = "/api/attendance/scan/" + rfidCard;
        
        client.print("GET " + url + " HTTP/1.1\r\n");
        client.print("Host: " + String(serverHost) + ":" + String(serverPort) + "\r\n");
        client.print("Connection: close\r\n\r\n");
        
        // Wait for response
        delay(1000);
        
        String response = "";
        while (client.available()) {
            char c = client.read();
            response += c;
        }
        
        Serial.println("Server Response:");
        Serial.println(response);
        
        // Parse response values
        String successStr = "false";
        String messageStr = "";
        String studentName = "";
        String parentPhone = "";
        
        // Extract success
        int successPos = response.indexOf("\"success\":");
        if (successPos > 0) {
            int valStart = successPos + 9;
            int valEnd = response.indexOf(",", valStart);
            if (valEnd < 0) valEnd = response.indexOf("}", valStart);
            successStr = response.substring(valStart, valEnd);
            successStr.replace("\"", "");
        }
        
        // Extract message
        int msgPos = response.indexOf("\"message\":");
        if (msgPos > 0) {
            int valStart = msgPos + 10;
            int valEnd = response.indexOf(",", valStart);
            if (valEnd < 0) valEnd = response.indexOf("}", valStart);
            messageStr = response.substring(valStart, valEnd);
            messageStr.replace("\"", "");
        }
        
        // Extract student_name
        int namePos = response.indexOf("\"student_name\":");
        if (namePos > 0) {
            int valStart = namePos + 15;
            int valEnd = response.indexOf(",", valStart);
            if (valEnd < 0) valEnd = response.indexOf("}", valStart);
            studentName = response.substring(valStart, valEnd);
            studentName.replace("\"", "");
        }
        
        // Extract parent_phone
        int phonePos = response.indexOf("\"parent_phone\":");
        if (phonePos > 0) {
            int valStart = phonePos + 15;
            int valEnd = response.indexOf(",", valStart);
            if (valEnd < 0) valEnd = response.indexOf("}", valStart);
            parentPhone = response.substring(valStart, valEnd);
            parentPhone.replace("\"", "");
        }
        
        // Output to SoftwareSerial: RFID|status|message|name|phone
        serialOut.print(rfidCard + "|" + successStr + "|" + messageStr + "|" + parentPhone+ "|" + studentName + "|");
        if (successStr == "true") {
            serialOut.print("|" + studentName + "|" + parentPhone);
        }
        // else{
        //     serialOut.println("Failed. Try Again");
        // }
        
        // Send feedback based on success
        if (successStr == "true") {
            // Success feedback
            digitalWrite(LED_SUCCESS, HIGH);
            Serial.println("Attendance marked successfully!");
            Serial.println("Student: " + studentName);
            Serial.println("Parent Phone: " + parentPhone);
            delay(2000);
            digitalWrite(LED_SUCCESS, LOW);
            
        } else {
            // Error feedback
            digitalWrite(LED_ERROR, HIGH);
            Serial.println("Attendance marking failed!");
            Serial.println("Message: " + messageStr);
            
            delay(2000);
            digitalWrite(LED_ERROR, LOW);
        }
        
        client.stop();
    } else {
        Serial.println("Connection to server failed!");
        digitalWrite(LED_ERROR, HIGH);
        delay(2000);
        digitalWrite(LED_ERROR, LOW);
    }
}
