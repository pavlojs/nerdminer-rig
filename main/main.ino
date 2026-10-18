#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "pool.h"
#include "secrets.h"
#include "Org_01.h"
#include "animations.h"

// --- OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_SDA D1
#define OLED_SCL D2
#define LED_PIN LED_BUILTIN
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- API ---
const bool CKPOOL = POOL_API == PoolApi::Ckpool;
const String statusURL = String(POOL_URL) + (CKPOOL ? "/pool/pool.status" : "/api/pool");
const String userURL = String(POOL_URL) + (CKPOOL ? "/users/" : "/api/client/") + WALLET_ADDR;

// --- Timers ---
unsigned long lastSwitch = 0;
const unsigned long pageInterval = 10000; // 10 seconds
int currentPage = 0;

// --- Stats ---
PoolStats poolStats;
UserStats userStats;

// --- Animation control ---
unsigned long animLastMillis0 = 0;
uint8_t animFrameIdx0 = 0;
unsigned long startupLastMs = 0;
uint8_t startupFrameIdx = 0;

// --- Time setting function ---
// the TZ rules switch daylight saving time on their own, SNTP keeps the clock synced in the background
void setupTime() {
  configTime(TIMEZONE, "pool.ntp.org", "time.nist.gov");

  // waiting up to 10 s for the first sync, any time before 2024 means NTP has not answered yet
  unsigned long start = millis();
  while (time(nullptr) < 1704067200 && millis() - start < 10000) {
    drawStartupFrame(); // still animating the screen
    delay(10);
  }
}

void blinkLED() {
  digitalWrite(LED_PIN, LOW);   // turn on LED
  delay(100);
  digitalWrite(LED_PIN, HIGH);  // turn off LED
}

  // draws the start frame at the center of the screen
void drawStartupFrame() {
  unsigned long now = millis();
  if (now - startupLastMs >= STARTUP_FRAME_DELAY) {
    startupLastMs = now;
    startupFrameIdx = (startupFrameIdx + 1) % STARTUP_FRAME_COUNT;
  }

  display.clearDisplay();

  // centering 48x48 on 128x64
  int x = (SCREEN_WIDTH - STARTUP_W) / 2;   // = 40
  int y = (SCREEN_HEIGHT - STARTUP_H) / 2;  // = 8

  display.drawBitmap(x, y, frames_startup[startupFrameIdx], STARTUP_W, STARTUP_H, SSD1306_WHITE);
  display.display();
}

// --- Functions fetching data from the API ---
// Returns the HTTP status code, or a negative HTTPClient error
int httpGet(const String& url, String& body) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  if (!http.begin(client, url)) return HTTPC_ERROR_CONNECTION_FAILED;
  int code = http.GET();
  if (code == HTTP_CODE_OK) body = http.getString();
  http.end();
  return code;
}

void fetchStats() {
  String body;
  int code = httpGet(statusURL, body);
  bool parsed = code == HTTP_CODE_OK &&
    (CKPOOL ? parseCkpoolStatus(body.c_str(), poolStats) : parsePublicPoolInfo(body.c_str(), poolStats));
  if (!parsed) {
    Serial.printf("Pool stats not updated (HTTP %d)\n", code);
  }
}

void fetchUserStats() {
  String body;
  int code = httpGet(userURL, body);
  if (CKPOOL && code == HTTP_CODE_NOT_FOUND) {
    // ckpool has no stats file for an address that has not mined there
    userStats = UserStats();
    return;
  }
  bool parsed = code == HTTP_CODE_OK &&
    (CKPOOL ? parseCkpoolUser(body.c_str(), userStats) : parsePublicPoolClient(body.c_str(), userStats));
  if (!parsed) {
    Serial.printf("User stats not updated (HTTP %d)\n", code);
  }
}

// prints text centered horizontally, with its baseline at y
void printCentered(const char* text, int16_t y) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2 - x1, y);
  display.print(text);
}

// --- Function drawing pages ---
void drawPage(int page) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  char buf[20];
  char value[16];
  time_t t = time(nullptr);
  struct tm timeinfo;
  localtime_r(&t, &timeinfo);

  if (page == 0) {

    display.setTextSize(2);
    display.setFont(&Org_01);
    display.setCursor(21, 12);
    display.println("NerdMiner");
    display.drawLine(0, 21, SCREEN_WIDTH, 21, SSD1306_WHITE);
    display.drawBitmap(5, 30, frames_clock[animFrameIdx0], CLOCK_W, CLOCK_H, SSD1306_WHITE);

    strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
    display.setCursor(52, 55);
    display.setTextSize(2);
    display.println(buf);

    strftime(buf, sizeof(buf), "%d-%m-%Y", &timeinfo);
    display.setTextSize(1);
    display.setCursor(52, 39);
    display.println(buf);

  } else if (page == 1) {

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
    display.setCursor(24, 33);
    display.println("YOUR HASHRATE:");
    display.setTextSize(2);
    display.setTextWrap(0);
    formatSuffixed(userStats.hashrate, "H/s", value, sizeof(value));
    printCentered(value, 50);

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
    display.drawBitmap(5, 27, frames_pool[animFrameIdx0], POOL_W, POOL_H, SSD1306_WHITE);
    formatSuffixed(poolStats.hashrate, "H/s", value, sizeof(value));
    if (CKPOOL) {
      display.setCursor(45, 35);
      display.printf("USERS: %ld", poolStats.users);
      display.setCursor(45, 45);
      display.printf("WORKERS: %ld", poolStats.workers);
      display.setCursor(45, 55);
      display.printf("--> %s", value);
    } else {
      display.setCursor(45, 40);
      display.printf("MINERS: %ld", poolStats.workers);
      display.setCursor(45, 50);
      display.printf("--> %s", value);
    }

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
    display.drawBitmap(5, 27, frames_user[animFrameIdx0], USER_W, USER_H, SSD1306_WHITE);
    if (CKPOOL) {
      display.setCursor(45, 35);
      display.printf("WORKERS: %ld", userStats.workers);
      display.setCursor(45, 45);
      formatSuffixed(userStats.shares, "", value, sizeof(value));
      display.printf("SHARES: %s", value);
      display.setCursor(45, 55);
      formatSuffixed(userStats.bestShare, "", value, sizeof(value));
      display.printf("BEST: %s", value);
    } else {
      display.setCursor(45, 40);
      display.printf("WORKERS: %ld", userStats.workers);
      display.setCursor(45, 50);
      formatSuffixed(userStats.bestShare, "", value, sizeof(value));
      display.printf("BEST: %s", value);
    }

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

  // animate until connected (without blocking delay)
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    // animate frames
    drawStartupFrame();

    // print dots every 500ms (non-blocking)
    static unsigned long dotLast = 0;
    if (millis() - dotLast >= 500) {
      Serial.print(".");
      dotLast = millis();
    }

    // allow WiFi threads to run
    yield();
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

    // refreshing animations every ~42ms
  static unsigned long lastDraw = 0;
  if (now - lastDraw >= 42) {
    lastDraw = now;

    if (currentPage == 0) {
      if (now - animLastMillis0 >= CLOCK_FRAME_DELAY) {
        animLastMillis0 = now;
        animFrameIdx0 = (animFrameIdx0 + 1) % CLOCK_FRAME_COUNT;
      }
    }
    if (currentPage == 2) {
      if (now - animLastMillis0 >= POOL_FRAME_DELAY) {
        animLastMillis0 = now;
        animFrameIdx0 = (animFrameIdx0 + 1) % POOL_FRAME_COUNT;
      }
    }
    if (currentPage == 3) {
      if (now - animLastMillis0 >= USER_FRAME_DELAY) {
        animLastMillis0 = now;
        animFrameIdx0 = (animFrameIdx0 + 1) % USER_FRAME_COUNT;
      }
    }

    // draw the current page (drawPage uses animFrameIdx0)
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