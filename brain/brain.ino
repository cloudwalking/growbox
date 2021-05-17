#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
//#import "growbox.h"

#define SYNC_PIN D6
//#define PWM_PIN 14 // D5
#define PWM_PIN D5

#define TIME_OFFSET (-7 * 60 * 60)
#define WAKE_TIME 8
#define AWAKE_DURATION_HOURS 14

unsigned long _currentTime = 0;
unsigned long _blinkTime = 0;
unsigned long _ntpSyncTime = 0;
bool _wifi_connected = false;
WiFiUDP _ntpUDP;
NTPClient _timeClient(_ntpUDP, "pool.ntp.org", TIME_OFFSET);

void setup() {
  Serial.begin(115200);
  pinMode(SYNC_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, true);

  delay(250);
  Serial.println("********");

  pinMode(PWM_PIN, OUTPUT);
  analogWriteRange(100);
  analogWriteFreq(25000);
  analogWrite(PWM_PIN, 15);
  
  WiFi.begin("cocacola", "football");

  delay(100);
}

void loop() {
  _currentTime = millis();

  blinkLED();

  if (WiFi.status() == WL_CONNECTED) {
    if (!_wifi_connected) {
      // Moving from not connected to connected.
      _wifi_connected = true;
      _timeClient.begin();
    }
    // Check time every 6 seconds.
    if (_ntpSyncTime == 0 || _currentTime - _ntpSyncTime > 6000) {
      _ntpSyncTime = _currentTime;
      _timeClient.update();

      printInfoNTP();
      updateRelayNTP();
    }
  }

  // Delay, allowing esp8266 to do other stuff.
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
  if (WiFi.status() != WL_CONNECTED) {
    dutyCycle = wifiErrorDutyCycle;
    divisor = errorDivisor;
  }

  long blinkCycleMS = 1000 * dutyCycle;
  digitalWrite(LED_BUILTIN, _currentTime % blinkCycleMS > blinkCycleMS / divisor);
}

void updateRelayNTP() {
//  ///// TEST MODE
//  int seconds = _timeClient.getSeconds();
//  int x = seconds / 10;
//  if (x == 0 || x == 2 || x == 4) {
//    Serial.println("ON !!!");
//    digitalWrite(SYNC_PIN, true);
//  } else {
//    Serial.println("OFF !!!");
//    digitalWrite(SYNC_PIN, false);
//  }
//  return;
//  ///// END TEST MODE

  int hour = _timeClient.getHours();
  int sleepTime = WAKE_TIME + AWAKE_DURATION_HOURS;
  
  Serial.print("wake time: ");
  Serial.print(WAKE_TIME);
  Serial.print(", sleep time: ");
  Serial.print(sleepTime);
  Serial.print(", current time: ");
  Serial.println(hour);
  
  if (hour >= WAKE_TIME && hour < sleepTime) {
    digitalWrite(SYNC_PIN, true);
    Serial.println("Turn relay ON");
  } else {
    digitalWrite(SYNC_PIN, false);
    Serial.println("Turn relay OFF");
  }
}

void printInfoNTP() {
  Serial.print("millis(): ");
  Serial.println(_currentTime);
  Serial.print("NTP day,hr:mn:sec : ");
  Serial.print(_timeClient.getDay());
  Serial.print(", ");
  Serial.print(_timeClient.getHours());
  Serial.print(":");
  Serial.print(_timeClient.getMinutes());
  Serial.print(":");
  Serial.println(_timeClient.getSeconds());
}
