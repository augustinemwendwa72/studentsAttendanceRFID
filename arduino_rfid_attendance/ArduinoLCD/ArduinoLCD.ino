#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

/* ---------- LCD ---------- */
#define LCD_CONTRAST 6
LiquidCrystal lcd(7, 5, 4, 3, 2, A0);
SoftwareSerial serialIn(8, 9);  // RX, TX

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
    lcdStatus(data.c_str());    
    Serial.println(data);
  }
}
