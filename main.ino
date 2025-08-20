#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "secrets.h"
#include "Org_01.h"

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_SDA D1
#define OLED_SCL D2
#define LED_PIN LED_BUILTIN
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- API ---
const char* apiURL = "https://pool.nerdminers.org/pool/pool.status";
String userApiURL = String("https://pool.nerdminers.org/users/") + WALLET_ADDR;

// --- Timers ---
unsigned long lastSwitch = 0;
const unsigned long pageInterval = 10000; // 10 seconds
int currentPage = 0;

// --- Stats ---
int usersCount = 0, workersCount = 0, idleCount = 0, disconnectedCount = 0;
String hashrate1m, hashrate5m, hashrate15m, hashrate1hr, hashrate6hr, hashrate1d, hashrate7d;
float diff = 0.0, accepted = 0, rejected = 0, bestshare = 0;
float SPS1m = 0, SPS5m = 0, SPS15m = 0, SPS1h = 0;

// --- User Stats ---
String u_hashrate1m, u_hashrate5m, u_hashrate1hr, u_hashrate1d, u_hashrate7d;
unsigned long u_lastshare = 0, u_authorised = 0;
int u_workers = 0;
float u_shares = 0.0, u_bestshare = 0.0, u_bestever = 0.0;

// --- Icons ---
const uint8_t bitmapclock[] = {0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x03, 0xff, 0xc0, 0x07, 0xc3, 0xe0, 0x0e, 0x00, 0x70, 0x1c, 0x18, 0x38, 0x38, 0x18, 0x1c, 0x30, 0x18, 0x0c, 0x70, 0x18, 0x0e, 0x70, 0x18, 0x0e, 0x60, 0x18, 0x06, 0x60, 0x1c, 0x06, 0x60, 0x1f, 0x06, 0x60, 0x0f, 0x86, 0x70, 0x03, 0x8e, 0x70, 0x00, 0x0e, 0x30, 0x00, 0x0c, 0x38, 0x00, 0x1c, 0x1c, 0x00, 0x38, 0x0e, 0x00, 0x70, 0x07, 0xc3, 0xe0, 0x03, 0xff, 0xc0, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00};
const uint8_t bitmappool[] = {0x00, 0xc3, 0x00, 0x00, 0xc3, 0x00, 0x00, 0xc3, 0x00, 0x0f, 0xff, 0xf0, 0x1f, 0xff, 0xf8, 0x18, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x18, 0xf8, 0xff, 0x1f, 0xf8, 0xff, 0x1f, 0x18, 0xc3, 0x18, 0x18, 0xc3, 0x18, 0x18, 0xc3, 0x18, 0xf8, 0xc3, 0x1f, 0xf8, 0xff, 0x1f, 0x18, 0xff, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x38, 0x1f, 0xff, 0xf8, 0x0f, 0xff, 0xf0, 0x00, 0xc3, 0x00, 0x00, 0xc3, 0x00, 0x00, 0xc3, 0x00};
const uint8_t bitmapstats[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0xc0, 0x00, 0x01, 0xe0, 0x00, 0x01, 0xe0, 0x00, 0x01, 0xe0, 0x00, 0x03, 0xf0, 0x00, 0x03, 0x30, 0x00, 0x03, 0x30, 0x00, 0x07, 0x38, 0x00, 0x7e, 0x18, 0x7e, 0x7e, 0x18, 0x7e, 0x00, 0x1c, 0xe0, 0x00, 0x0c, 0xc0, 0x00, 0x0c, 0xc0, 0x00, 0x0f, 0xc0, 0x00, 0x07, 0x80, 0x00, 0x07, 0x80, 0x00, 0x07, 0x80, 0x00, 0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// --- Time setting function ---
void setupTime() {
  long offsetSeconds = UTC_OFFSET * 3600;
  long dstOffset = 0;

  configTime(offsetSeconds, 0, "pool.ntp.org", "time.nist.gov");
  delay(2000);

  time_t now = time(nullptr);
  struct tm tmNow;
  localtime_r(&now, &tmNow);
  int month = tmNow.tm_mon + 1;

  if (strcmp(DST_REGION, "EU") == 0) {
    if (month > 3 && month < 10) dstOffset = 3600;
  } else if (strcmp(DST_REGION, "US") == 0) {
    if (month > 3 && month < 11) dstOffset = 3600;
  }

  configTime(offsetSeconds + dstOffset, 0, "pool.ntp.org", "time.nist.gov");
}

void blinkLED() {
  digitalWrite(LED_PIN, LOW);   // turn on LED
  delay(100);
  digitalWrite(LED_PIN, HIGH);  // turn off LED
}

// --- Function fetching data from the API ---
void fetchStats() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if (http.begin(client, apiURL)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);

      // --- Merge JSON into an array ---
      String json1, json2, json3;
      int firstNL = payload.indexOf('\n');
      int secondNL = payload.indexOf('\n', firstNL + 1);
      if (firstNL > 0 && secondNL > 0) {
        json1 = payload.substring(0, firstNL);
        json2 = payload.substring(firstNL + 1, secondNL);
        json3 = payload.substring(secondNL + 1);
      }
      String jsonArray = "[" + json1 + "," + json2 + "," + json3 + "]";

      // --- Parsing ---
      StaticJsonDocument<4096> doc;
      DeserializationError error = deserializeJson(doc, jsonArray);
      if (error) {
        Serial.print("Błąd JSON: ");
        Serial.println(error.c_str());
        http.end();
        return;
      }

      // --- Data from all 3 objects ---
      usersCount = doc[0]["Users"] | 0;
      workersCount = doc[0]["Workers"] | 0;
      idleCount = doc[0]["Idle"] | 0;
      disconnectedCount = doc[0]["Disconnected"] | 0;

      hashrate1m = doc[1]["hashrate1m"].as<String>();
      hashrate5m = doc[1]["hashrate5m"].as<String>();
      hashrate15m = doc[1]["hashrate15m"].as<String>();
      hashrate1hr = doc[1]["hashrate1hr"].as<String>();
      hashrate6hr = doc[1]["hashrate6hr"].as<String>();
      hashrate1d = doc[1]["hashrate1d"].as<String>();
      hashrate7d = doc[1]["hashrate7d"].as<String>();

      diff = doc[2]["diff"] | 0.0;
      accepted = doc[2]["accepted"] | 0.0;
      rejected = doc[2]["rejected"] | 0.0;
      bestshare = doc[2]["bestshare"] | 0.0;
      SPS1m = doc[2]["SPS1m"] | 0.0;
      SPS5m = doc[2]["SPS5m"] | 0.0;
      SPS15m = doc[2]["SPS15m"] | 0.0;
      SPS1h = doc[2]["SPS1h"] | 0.0;
    }
    http.end();
  }
}

void fetchUserStats() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if (http.begin(client, userApiURL)) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);

      StaticJsonDocument<4096> doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print("Błąd JSON user: ");
        Serial.println(error.c_str());
        http.end();
        return;
      }

      u_hashrate1m = doc["hashrate1m"].as<String>();
      u_hashrate5m = doc["hashrate5m"].as<String>();
      u_hashrate1hr = doc["hashrate1hr"].as<String>();
      u_hashrate1d = doc["hashrate1d"].as<String>();
      u_hashrate7d = doc["hashrate7d"].as<String>();
      u_lastshare  = doc["lastshare"] | 0;
      u_workers    = doc["workers"] | 0;
      u_shares     = doc["shares"] | 0.0;
      u_bestshare  = doc["bestshare"] | 0.0;
      u_bestever   = doc["bestever"] | 0.0;
      u_authorised = doc["authorised"] | 0;
    }
    http.end();
  }
}

// --- Function drawing pages ---
void drawPage(int page) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  char buf[20];
  time_t t = time(nullptr);
  struct tm timeinfo;
  localtime_r(&t, &timeinfo);

  if (page == 0) {

    display.setTextSize(2);
    display.setFont(&Org_01);
    display.setCursor(21, 12);
    display.println("NerdMiner");
    display.drawLine(0, 21, SCREEN_WIDTH, 21, SSD1306_WHITE);
    display.drawBitmap(10, 35, bitmapclock, 24, 24, 1);

    strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
    display.setCursor(50, 55);
    display.setTextSize(2);
    display.println(buf);

    strftime(buf, sizeof(buf), "%d-%m-%Y", &timeinfo);
    display.setTextSize(1);
    display.setCursor(50, 39);
    display.println(buf);

  } else if (page == 1) {

    // display.println("Users: " + String(usersCount));
    // display.println("Workers: " + String(workersCount));
    // display.println("Idle: " + String(idleCount));
    // display.println("Disconnected: " + String(disconnectedCount));
    // display.println("Diff: " + String(diff));
    // display.println("Accepted: " + String(accepted));
    // display.println("Rejected: " + String(rejected));
    // display.println("BestShare: " + String(bestshare));
    // display.println("SPS 1m: " + String(SPS1m));
    // display.println("SPS 5m: " + String(SPS5m));
    // display.println("SPS 15m: " + String(SPS15m));
    // display.println("SPS 1h: " + String(SPS1h));

    display.setTextSize(2);
    display.setFont(&Org_01);
    display.setCursor(0, 11);
    display.setTextWrap(0);
    display.setCursor(21, 11);
    display.println("NerdMiner");
    display.drawRect(0, 22, 128, 42, 1);
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.setTextWrap(0);
    display.setCursor(24, 34);
    display.println("POLL HASHRATE:");
    display.setCursor(0, 50);
    display.setTextSize(2);
    display.setTextWrap(0);
    display.setCursor(24, 50);
    display.println(hashrate1m + "H/s");

  } else if (page == 2) {

    display.setTextSize(2);
    display.setFont(&Org_01);
    display.setCursor(0, 11);
    display.setTextWrap(0);
    display.setCursor(13, 11);
    display.println("Pool Stats");
    display.drawRect(0, 22, 128, 42, 1);
    display.setTextSize(1);
    display.setCursor(0, 34);
    display.setTextSize(1);
    display.drawBitmap(10, 31, bitmappool, 24, 24, 1);
    display.setCursor(45, 35);
    display.println("USERS: " + String(usersCount));
    display.setCursor(45, 45);
    display.println("WORKERS: " + String(workersCount));
    display.setCursor(45, 55);
    display.println("DIFF: " + String(diff));

  } else if (page == 3) {

    display.setTextSize(2);
    display.setFont(&Org_01);
    display.setCursor(0, 11);
    display.setTextWrap(0);
    display.setCursor(10, 11);
    display.println("Your Stats");
    display.drawRect(0, 22, 128, 42, 1);
    display.setTextSize(1);
    display.setCursor(0, 34);
    display.setTextSize(1);
    display.drawBitmap(10, 31, bitmapstats, 24, 24, 1);
    display.setCursor(45, 35);
    display.println("WORKERS: " + String(u_workers));
    display.setCursor(45, 45);
    display.println("SHARES: " + String(u_shares, 0));
    display.setCursor(45, 55);
    display.println("BESTSHARE: " + String(u_bestshare, 2));

  }
  display.display();
}

// --- Setup ---
void setup() {
  Serial.begin(115200);

  // --- OLED ---
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.clearDisplay();
  display.display();

  // --- LED ---
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // disabled at startup

  // --- WiFi ---
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  setupTime();
  fetchStats();
  fetchUserStats();
  blinkLED();
}

// --- Loop ---
void loop() {
  unsigned long now = millis();
  if (now - lastSwitch > pageInterval) {
    lastSwitch = now;
    currentPage++;
    if (currentPage > 3) currentPage = 0;
    drawPage(currentPage);
  }

  static unsigned long lastFetch = 0;
  if (millis() - lastFetch > 60000) { // refreshing every minute
    lastFetch = millis();
    fetchStats();
    fetchUserStats();
    blinkLED();
  }
}
