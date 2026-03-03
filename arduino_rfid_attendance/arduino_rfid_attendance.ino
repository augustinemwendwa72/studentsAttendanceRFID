/*
 * Student Attendance RFID Scanner
 * Arduino Mega + ESP8266 + RFID RC522
 * 
 * This code reads RFID cards and sends attendance data to the server
 */

#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>

// ==================== PIN CONFIGURATION ====================
// Arduino Mega pins for RFID RC522
#define RST_PIN 5    // Pin 5 for RFID RST
#define SS_PIN 53    // Pin 53 for RFID SS (SDA)

// Software Serial for ESP8266 communication with Arduino
// Using pins 10 (RX) and 11 (TX)
#define ESP_RX 10
#define ESP_TX 11

// ==================== WIFI CONFIGURATION ====================
const char* ssid = "YOUR_WIFI_SSID";          // Replace with your WiFi SSID
const char* password = "YOUR_WIFI_PASSWORD";  // Replace with your WiFi password

// ==================== SERVER CONFIGURATION ====================
// Replace with your server IP address or domain
// If running on local network, use the computer's IP address
const char* serverHost = "192.168.1.100";    // Replace with server IP
const int serverPort = 3000;

// ==================== VARIABLES ====================
SoftwareSerial espSerial(ESP_RX, ESP_TX);  // RX, TX
MFRC522 rfid(SS_PIN, RST_PIN);              // Create MFRC522 instance

String uidString = "";
bool cardPresent = false;
bool lastCardState = false;

// LED indicators
#define LED_WIFI 2      // Pin 2 - WiFi status LED
#define LED_SUCCESS 3   // Pin 3 - Success LED
#define LED_ERROR 4     // Pin 4 - Error LED
#define BUZZER 6        // Pin 6 - Buzzer

void setup() {
    Serial.begin(9600);     // Debug serial
    espSerial.begin(9600);  // ESP8266 communication
    
    SPI.begin();            // Initialize SPI bus
    
    // Initialize RFID
    rfid.PCD_Init();
    Serial.println("RFID Reader initialized");
    
    // Initialize LED pins
    pinMode(LED_WIFI, OUTPUT);
    pinMode(LED_SUCCESS, OUTPUT);
    pinMode(LED_ERROR, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    
    // Turn off LEDs initially
    digitalWrite(LED_WIFI, LOW);
    digitalWrite(LED_SUCCESS, LOW);
    digitalWrite(LED_ERROR, LOW);
    digitalWrite(BUZZER, LOW);
    
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
            uidString = "";
            for (byte i = 0; i < rfid.uid.size; i++) {
                uidString += String(rfid.uid.uidByte[i], HEX);
            }
            uidString.toUpperCase();
            
            Serial.print("Card detected: ");
            Serial.println(uidString);
            
            // Send attendance to server
            sendAttendance(uidString);
            
            // Halt PICC
            rfid.PICC_HaltA();
        }
    }
    
    delay(100);  // Small delay
}

// ==================== WIFI CONNECTION ====================
void connectWiFi() {
    Serial.println("Connecting to WiFi...");
    
    // Send AT command to ESP8266
    espSerial.println("AT");
    delay(1000);
    printESPResponse();
    
    // Set WiFi mode
    espSerial.println("AT+CWMODE=1");
    delay(1000);
    printESPResponse();
    
    // Connect to WiFi
    String cmd = "AT+CWJAP=\"";
    cmd += ssid;
    cmd += "\",\"";
    cmd += password;
    cmd += "\"";
    espSerial.println(cmd);
    
    Serial.println("Connecting...");
    int timeout = 30;
    while (timeout > 0) {
        delay(1000);
        Serial.print(".");
        timeout--;
        
        // Check for connected response
        if (espSerial.find("OK")) {
            Serial.println("\nWiFi Connected!");
            digitalWrite(LED_WIFI, HIGH);  // Turn on WiFi LED
            return;
        }
    }
    
    Serial.println("\nWiFi connection failed!");
    digitalWrite(LED_ERROR, HIGH);
}

// ==================== SEND ATTENDANCE TO SERVER ====================
void sendAttendance(String rfidCard) {
    Serial.println("Sending attendance to server...");
    
    // Build HTTP request
    String url = "/api/attendance/scan/" + rfidCard;
    
    String request = "GET " + url + " HTTP/1.1\r\n";
    request += "Host: " + String(serverHost) + "\r\n";
    request += "Connection: close\r\n\r\n";
    
    // Connect to server
    espSerial.print("AT+CIPSTART=\"TCP\",\"");
    espSerial.print(serverHost);
    espSerial.print("\",");
    espSerial.println(serverPort);
    
    delay(2000);
    printESPResponse();
    
    // Send data length
    String cipSend = "AT+CIPSEND=";
    cipSend += request.length();
    espSerial.println(cipSend);
    
    delay(1000);
    printESPResponse();
    
    // Send HTTP request
    espSerial.print(request);
    
    // Wait for response
    delay(3000);
    String response = "";
    while (espSerial.available()) {
        response += (char)espSerial.read();
    }
    
    Serial.println("Server Response:");
    Serial.println(response);
    
    // Parse response
    parseResponse(response);
}

// ==================== PARSE SERVER RESPONSE ====================
void parseResponse(String response) {
    // Check if response contains "success"
    if (response.indexOf("\"success\":true") > 0 || response.indexOf("success") > 0) {
        // Extract parent phone and message
        int phoneStart = response.indexOf("parent_phone") + 15;
        int phoneEnd = response.indexOf("\"", phoneStart);
        String parentPhone = response.substring(phoneStart, phoneEnd);
        
        int msgStart = response.indexOf("message") + 10;
        int msgEnd = response.indexOf("\"", msgStart);
        String message = response.substring(msgStart, msgEnd);
        
        // Success indicators
        digitalWrite(LED_SUCCESS, HIGH);
        digitalWrite(BUZZER, HIGH);
        delay(200);
        digitalWrite(BUZZER, LOW);
        
        Serial.println("Attendance marked successfully!");
        Serial.println("Parent Phone: " + parentPhone);
        Serial.println("Message: " + message);
        
        // You can add SMS sending code here
        
        delay(1000);
        digitalWrite(LED_SUCCESS, LOW);
    } else {
        // Error indicators
        digitalWrite(LED_ERROR, HIGH);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        
        Serial.println("Attendance marking failed!");
        
        delay(1000);
        digitalWrite(LED_ERROR, LOW);
    }
}

// ==================== DEBUG HELPER ====================
void printESPResponse() {
    while (espSerial.available()) {
        Serial.write(espSerial.read());
    }
}
