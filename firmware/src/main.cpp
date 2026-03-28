#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

// Button wiring:
// one side of button -> GPIO 4
// other side of button -> GND
const int BUTTON_PIN = 4;

// Optional LED for feedback
const int LED_PIN = 2;

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 250;

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Connected!");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP());
}

void sendSleepRequest() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi not connected");
    return;
  }

  HTTPClient http;
  http.begin(SLEEP_URL);

  int httpCode = http.GET();

  Serial.print("HTTP response code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("Response body: ");
    Serial.println(response);

    // quick LED blink on success
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
  } else {
    Serial.print("Request failed, error: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  connectWifi();
}

void loop() {
  bool currentButtonState = digitalRead(BUTTON_PIN);

  // detect fresh press: HIGH -> LOW
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      Serial.println("Button pressed");
      sendSleepRequest();
    }
  }

  lastButtonState = currentButtonState;
  delay(20);
}
