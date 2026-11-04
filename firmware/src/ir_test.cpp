// IR loopback test
//
// Validates both halves of the IR discovery system on one board (or two).
// Every 2 seconds it transmits a 32-bit NEC code through the IR LED, and it
// continuously prints anything the TSOP38238 receiver decodes.
//
// Single-board loopback: point the IR LED at the TSOP (a few cm apart, or
// bounce off a white wall/your hand) and you should see your own codes echoed
// back on the serial monitor.
//
// Wiring:
//   IR LED (TX):  GPIO 25 -> 1k -> 2N2222 base, emitter -> GND,
//                 3.3V -> IR LED -> 22-100R -> collector
//   TSOP38238:    OUT -> GPIO 26, GND -> GND, VS -> 3.3V
//   Haptic motor: GPIO 18 -> base resistor -> transistor, motor between 3.3V
//                 and collector, flyback diode across motor (band to 3.3V).
//                 Buzzes briefly whenever a valid payload is received.
//
// Build & flash:  pio run -e ir_test -t upload
// Monitor:        pio device monitor

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

const uint16_t IR_TX_PIN = 25;
const uint16_t IR_RX_PIN = 26;
const uint8_t MOTOR_PIN = 18;  // same pin main.cpp uses

const unsigned long buzzDurationMs = 150;
unsigned long motorOffTime = 0;

// 0xA5 marker byte so we can tell our codes apart from a stray TV remote.
const uint32_t PAYLOAD_MARKER = 0xA5000000;

IRsend irsend(IR_TX_PIN);
IRrecv irrecv(IR_RX_PIN);
decode_results results;

unsigned long lastSendTime = 0;
const unsigned long sendIntervalMs = 2000;
uint16_t counter = 0;

void setup() {
  Serial.begin(115200);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);
  irsend.begin();
  irrecv.enableIRIn();
  Serial.println();
  Serial.println("IR loopback test: TX on GPIO 25, RX (TSOP) on GPIO 26");

  // motor self-test: three buzzes at boot, independent of IR
  Serial.println("Motor self-test: 3 buzzes on GPIO 18...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(MOTOR_PIN, HIGH);
    delay(150);
    digitalWrite(MOTOR_PIN, LOW);
    delay(200);
  }
  Serial.println("Motor self-test done.");
}

void loop() {
  if (millis() - lastSendTime >= sendIntervalMs) {
    lastSendTime = millis();
    uint32_t payload = PAYLOAD_MARKER | counter;
    counter++;
    Serial.printf("TX  NEC 0x%08X\n", payload);
    irsend.sendNEC(payload, 32);
  }

  if (irrecv.decode(&results)) {
    uint32_t value = (uint32_t)results.value;
    bool ours = (value & 0xFF000000) == PAYLOAD_MARKER;
    Serial.printf("RX  %s 0x%08X (%d bits)%s\n",
                  typeToString(results.decode_type).c_str(),
                  value, results.bits,
                  ours ? "  <- our payload" : "");
    if (ours) {
      // non-blocking buzz so IR receive timing is unaffected
      digitalWrite(MOTOR_PIN, HIGH);
      motorOffTime = millis() + buzzDurationMs;
    }
    irrecv.resume();
  }

  if (motorOffTime != 0 && millis() >= motorOffTime) {
    digitalWrite(MOTOR_PIN, LOW);
    motorOffTime = 0;
  }
}
