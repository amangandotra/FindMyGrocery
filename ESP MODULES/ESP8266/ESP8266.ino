/*
 * Library Rack LED Controller - WiFi-connect RGB animation on startup
 */

#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h> 

// -------------------
// CONFIGURATION
// -------------------

const char* ssid     = "SmartLibrary";
const char* password = "";
const char* hostName = "rack-b";
// TODO CHANGE RACK HERE FOR EVERY RACK
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define NUM_STRIPS  5

#define RACK_LEDS   10
#define GLOW_LEDS   4

#define BRIGHTNESS  255

// PIN DEFINITIONS
#define PIN_R1_LEFT    5    // D1
#define PIN_R2_LEFT    4    // D2
#define PIN_R1_RIGHT   16   // D0
#define PIN_R2_RIGHT   12   // D6
#define PIN_GLOW_RACK  13   // D7

CRGB leds[4][RACK_LEDS];      // Main racks
CRGB glowRack[GLOW_LEDS];     // Glow rack

ESP8266WebServer server(80);

// -------------------
// FUNCTION DECLARATIONS
// -------------------

void connectWithRgbAnimation();
void fillAll(CRGB baseColor);
void fillAllWithBrightness(CRGB baseColor, uint8_t bri);
void fillSegment(int stripIdx, int start, int end, CRGB baseColor, int brightness);
void fillGlowRack(CRGB baseColor, int brightness);
void clearGlowRack();
void runBlinkAnimation(int stripIdx, int start, int end, CRGB color);
void runGlowRackAnimation(CRGB color);
void sendIpToServer();
// -------------------
// SETUP
// -------------------

void setup() {
  Serial.begin(115200);

  // PIN STABILIZATION
  pinMode(PIN_R1_LEFT, OUTPUT);    digitalWrite(PIN_R1_LEFT, LOW);
  pinMode(PIN_R2_LEFT, OUTPUT);    digitalWrite(PIN_R2_LEFT, LOW);
  pinMode(PIN_R1_RIGHT, OUTPUT);   digitalWrite(PIN_R1_RIGHT, LOW);
  pinMode(PIN_R2_RIGHT, OUTPUT);   digitalWrite(PIN_R2_RIGHT, LOW);
  pinMode(PIN_GLOW_RACK, OUTPUT);  digitalWrite(PIN_GLOW_RACK, LOW);

  delay(100);

  // LED SETUP
  FastLED.addLeds<LED_TYPE, PIN_R1_LEFT,  COLOR_ORDER>(leds[0], RACK_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_R2_LEFT,  COLOR_ORDER>(leds[1], RACK_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_R1_RIGHT, COLOR_ORDER>(leds[2], RACK_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_R2_RIGHT, COLOR_ORDER>(leds[3], RACK_LEDS);
  FastLED.addLeds<LED_TYPE, PIN_GLOW_RACK, COLOR_ORDER>(glowRack, GLOW_LEDS);

  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear(true);
  FastLED.show();

  // Start WiFi and animate RGB while connecting
  Serial.println("\nConnecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  // This function will animate until WiFi connects, then fade out
  connectWithRgbAnimation();

  // At this point WiFi is connected
  Serial.println("\nWiFi Connected.");
  sendIpToServer();  
  if (MDNS.begin(hostName)) {
    Serial.println("mDNS started");
  }

  // ROUTES
  server.on("/", handleRoot);
  server.on("/highlight", handleHighlight);
  server.on("/glowrack", handleGlowRack);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  MDNS.update();
}

// -------------------
// CONNECT ANIMATION
// -------------------

void connectWithRgbAnimation() {
  // Rotate hue while WiFi is not connected
  // When connected we do a short fade-out.
  uint32_t lastShow = 0;
  const uint16_t animDelay = 40; // ms between frames
  uint8_t hue = 0;

  while (WiFi.status() != WL_CONNECTED) {
    uint32_t now = millis();
    if (now - lastShow >= animDelay) {
      lastShow = now;
      // smooth-ish color cycle using CHSV
      CHSV hsvColor(hue, 255, 160); // value ~160 so it's not blinding; change if needed
      fillAll(CRGB(hsvColor));
      FastLED.show();
      hue++; // advance hue slowly
    }
    // Allow background WiFi tasks
    yield();
    // Also print dots occasionally so serial shows progress
    static uint32_t lastDot = 0;
    if (millis() - lastDot > 1000) {
      Serial.print(".");
      lastDot = millis();
    }
  }

  // Fade out quickly once connected (so all LEDs turn off cleanly)
  for (int v = 160; v >= 0; v -= 16) {
    CHSV hsvColor(hue, 255, (uint8_t)max(0, v));
    fillAll(CRGB(hsvColor));
    FastLED.show();
    delay(30);
    yield();
  }

  FastLED.clear(true);
  FastLED.show();
}

// -------------------
// API HANDLERS
// -------------------
void sendIpToServer() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;                  // ← Create client object
  HTTPClient http;

  String url = "http://fmb.local:5000/register_ip";
  http.begin(client, url);           // ← Pass client + URL here
  http.addHeader("Content-Type", "application/json");
// TODO CHANGE RACK A B C D HERE
  String json = "{\"device\":\"rackb\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  int httpResponseCode = http.POST(json);

  Serial.print("IP Register Response: ");
  Serial.println(httpResponseCode);

  http.end();
}


void handleRoot() {
  server.send(200, "text/plain", "Controller Online");
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

void handleHighlight() {
  if (!server.hasArg("row") || !server.hasArg("side") || !server.hasArg("col")) {
    server.send(400, "text/plain", "Missing args");
    return;
  }

  int row  = server.arg("row").toInt();
  String side = server.arg("side");
  int col  = server.arg("col").toInt();

  int r = server.arg("r").toInt();
  int g = server.arg("g").toInt();
  int b = server.arg("b").toInt();

  int stripIndex = -1;

  if (row == 1 && side == "left")       stripIndex = 0;
  else if (row == 2 && side == "left")  stripIndex = 1;
  else if (row == 1 && side == "right") stripIndex = 2;
  else if (row == 2 && side == "right") stripIndex = 3;

  if (stripIndex == -1 || col < 1 || col > 2) {
    server.send(400, "text/plain", "Invalid input");
    return;
  }

  int startLed = (col == 1) ? 0 : 5;
  int endLed   = (col == 1) ? 4 : 9;

  server.send(200, "text/plain", "OK");

  runBlinkAnimation(stripIndex, startLed, endLed, CRGB(r, g, b));
}

void handleGlowRack() {
  if (!server.hasArg("r") || !server.hasArg("g") || !server.hasArg("b")) {
    server.send(400, "text/plain", "Missing RGB args");
    return;
  }

  int r = server.arg("r").toInt();
  int g = server.arg("g").toInt();
  int b = server.arg("b").toInt();

  server.send(200, "text/plain", "OK");

  runGlowRackAnimation(CRGB(r, g, b));
}

// -------------------
// ANIMATIONS
// -------------------

void runBlinkAnimation(int stripIdx, int start, int end, CRGB color) {
  FastLED.clear();
  FastLED.show();

  for (int k = 0; k < 3; k++) {

    for (int b = 0; b <= 255; b += 15) {
      fillSegment(stripIdx, start, end, color, b);
      noInterrupts();
      FastLED.show();
      interrupts();
      delay(15);
      yield();
    }

    delay(300);

    for (int b = 255; b >= 0; b -= 15) {
      fillSegment(stripIdx, start, end, color, b);
      noInterrupts();
      FastLED.show();
      interrupts();
      delay(15);
      yield();
    }

    delay(150);
  }

  FastLED.clear();
  FastLED.show();
}

void runGlowRackAnimation(CRGB color) {
  for (int k = 0; k < 3; k++) {

    for (int b = 0; b <= 255; b += 15) {
      fillGlowRack(color, b);
      noInterrupts();
      FastLED.show();
      interrupts();
      delay(15);
      yield();
    }

    delay(300);

    for (int b = 255; b >= 0; b -= 15) {
      fillGlowRack(color, b);
      noInterrupts();
      FastLED.show();
      interrupts();
      delay(15);
      yield();
    }

    delay(150);
  }

  clearGlowRack();
}

// -------------------
// HELPERS
// -------------------

void fillSegment(int stripIdx, int start, int end, CRGB baseColor, int brightness) {
  CRGB dimmed = baseColor;
  dimmed.nscale8(brightness);

  for (int i = start; i <= end; i++) {
    leds[stripIdx][i] = dimmed;
  }
}

void fillGlowRack(CRGB baseColor, int brightness) {
  CRGB dimmed = baseColor;
  dimmed.nscale8(brightness);

  for (int i = 0; i < GLOW_LEDS; i++) {
    glowRack[i] = dimmed;
  }
}

void clearGlowRack() {
  for (int i = 0; i < GLOW_LEDS; i++) {
    glowRack[i] = CRGB::Black;
  }
  FastLED.show();
}

// Fill all LEDs (racks + glow) with the provided color
void fillAll(CRGB baseColor) {
  for (int s = 0; s < 4; s++) {
    for (int i = 0; i < RACK_LEDS; i++) {
      leds[s][i] = baseColor;
    }
  }
  for (int i = 0; i < GLOW_LEDS; i++) {
    glowRack[i] = baseColor;
  }
}

// Fill all with color but using brightness scaling (0-255)
void fillAllWithBrightness(CRGB baseColor, uint8_t bri) {
  CRGB dimmed = baseColor;
  dimmed.nscale8_video(bri);
  fillAll(dimmed);
}
