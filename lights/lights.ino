#include <Wire.h>
#include "SparkFun_Qwiic_Relay.h"

#define SYNC_PIN D6
#define RELAY_ADDR 0x18

Qwiic_Relay _relay(RELAY_ADDR); 
unsigned long _currentTime = 0;
int _relayState = 0;

void setup() {
  delay(250);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SYNC_PIN, INPUT);

  Serial.begin(115200);
  
  Wire.begin();
  if (_relay.begin()) {
    Serial.println("Connected to relay.");
  } else {
    Serial.println("RELAY NOT FOUND");
  }
  _relay.turnRelayOff();
}

void loop() {
  _currentTime = millis();
  _relayState = _relay.getState();
  blinkLED();

  if (digitalRead(SYNC_PIN)) {
    if (_relayState != 1) {
      _relay.turnRelayOn();
    }
  } else {
    if (_relayState != 10) {
      _relay.turnRelayOff();
    }
  }
  delay(100);
}

void blinkLED() {
  const float wifiErrorDutyCycle = 0.5;
  const float relayErrorDutyCycle = 1;
  const float errorDivisor = 2;
  const float nominalDutyCycle = 10;
  const float nominalDivisor = 150;

  float dutyCycle = nominalDutyCycle;
  float divisor = nominalDivisor;
  if (_relayState == 255) {
    // Sometimes this sparkfun qwiic relay returns 255 for its state, which indicates the
    // relay is not connected or malfunctioning. e.g. if you've changed the PWM settings :)
    dutyCycle = relayErrorDutyCycle;
    divisor = errorDivisor;
  }

  long blinkCycleMS = 1000 * dutyCycle;
  digitalWrite(LED_BUILTIN, _currentTime % blinkCycleMS > blinkCycleMS / divisor);
}
