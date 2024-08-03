#include <SPI.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>

ESP8266WiFiMulti WiFiMulti;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#include "config.h"

#ifndef WIFI_SSID
#error WiFi SSID needs to be provided
#endif

#ifndef WIFI_PASSWORD
#error Wifi password needs to be provided
#endif

#ifndef TIBBER_TOKEN
#error Tibber token needs to be provided
#endif

auto client = std::make_unique<BearSSL::WiFiClientSecure>();

void setup() {
  Serial.begin(9600);

  setupDisplay();

  setupWifi();

  client->setInsecure(); // or https://forum.arduino.cc/t/esp8266-httpclient-library-for-https/495245
}

void setupWifi() {
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  display.clearDisplay();

  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.setCursor(10, 10);
  display.println("Connecting...");
  display.display();

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
  }

  display.clearDisplay();

  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.setCursor(10, 10);
  display.println(WiFi.localIP());
  display.display();
}

void setupDisplay() {
   // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  // Clear the buffer
  display.clearDisplay();

  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);

  display.println("Loading...");

  display.display();

  delay(1000);
}

void loadCurrentPrice() {
  display.clearDisplay();
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
  display.setCursor(10, 20);
  display.setTextSize(1);

  HTTPClient https;
  if (https.begin(*client, "https://api.tibber.com/v1-beta/gql")) {
    https.addHeader("Authorization", String("Bearer ") + String(TIBBER_TOKEN), false, false);
    https.addHeader("Content-Type", "application/json");

    String payload = "{\"query\": \"{viewer {homes {currentSubscription {priceInfo {current {total}}}}}}\"}";

    int httpCode = https.POST(payload);

    // httpCode will be negative on error
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        // display.println(payload);

        JsonDocument doc;
        DeserializationError jsonError = deserializeJson(doc, payload);

        // Test if parsing succeeds.
        if (jsonError) {
          display.println(jsonError.f_str());
          display.display();
          return;
        }

        double currentPrice = doc["data"]["viewer"]["homes"][0]["currentSubscription"]["priceInfo"]["current"]["total"];
        display.setCursor(10, 10);
        display.setTextSize(1);
        display.println("Current price:");

        display.setCursor(20, 30);
        display.setTextSize(3);
        display.println(String(currentPrice * 100, 0) + " ct");
      }
    } else {
      display.println(https.errorToString(httpCode));
    }

    https.end();
  } else {
    display.println("Unable to connect.");
  }

  display.display();
}

void loop() {
  loadCurrentPrice();
  delay(1000 * 60);
}
