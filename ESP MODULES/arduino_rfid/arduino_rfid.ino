#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>

#define SS_COUNTER 10
#define SS_DOOR    8
#define RST_PIN    9

#define LED_PIN    6
#define NUM_LEDS   2
#define BUZZER_PIN 3

MFRC522 rfidCounter(SS_COUNTER, RST_PIN);
MFRC522 rfidDoor(SS_DOOR, RST_PIN);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastScanCounter = 0;
unsigned long lastScanDoor = 0;
String serialBuffer = "";

void setup() {
  Serial.begin(115200);
  SPI.begin();

  rfidCounter.PCD_Init();
  rfidDoor.PCD_Init();

  strip.begin();
  strip.show();  // Initialize all LEDs to off

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void readReader(MFRC522 &rfid, const char *source, unsigned long &lastScan)
{
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  if (millis() - lastScan < 1500) return;
  lastScan = millis();

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("RFID:");
  Serial.print(source);
  Serial.print(":");
  Serial.println(uid);

  rfid.PICC_HaltA();
}

void shortBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(150);
  digitalWrite(BUZZER_PIN, LOW);
}

void theftAlert() {
  Serial.println("ALERT: Theft pattern triggered");
  for (int i = 0; i < 5; i++) {
    strip.fill(strip.Color(255, 0, 0));  // Red
    strip.show();
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);

    strip.clear();
    strip.show();
    digitalWrite(BUZZER_PIN, LOW);
    delay(300);
  }
}

void showIssued() {
  Serial.println("INFO: Book issued pattern triggered");
  strip.fill(strip.Color(0, 255, 0));  // Green
  strip.show();
  shortBeep();
  delay(800);
  strip.clear();
  strip.show();
}

void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer.trim();

      if (serialBuffer.length() > 0) {
        Serial.print("CMD RECEIVED: ");
        Serial.println(serialBuffer);
      }

      if (serialBuffer == "LED:ISSUED") {
        showIssued();
      } else if (serialBuffer == "LED:THEFT") {
        theftAlert();
      } else {
        Serial.print("WARN: Unknown command - ");
        Serial.println(serialBuffer);
      }

      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}

void loop() {
  readReader(rfidCounter, "COUNTER", lastScanCounter);
  readReader(rfidDoor, "DOOR", lastScanDoor);
  handleSerialInput();  // Non-blocking serial command handler
}
