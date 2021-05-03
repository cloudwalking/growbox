#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <SparkFun_Qwiic_Relay.h>

#define PWM_PIN 14 // D5
#define RELAY_ADDR 0x18
#define TIME_OFFSET (-7 * 60 * 60)

#define WAKE_TIME 8
#define AWAKE_DURATION_HOURS 14

unsigned long _currentTime = 0;
unsigned long _blinkTime = 0;
unsigned long _ntpSyncTime = 0;
bool _wifi_connected = false;
Qwiic_Relay _relay(RELAY_ADDR);
WiFiUDP _ntpUDP;
NTPClient _timeClient(_ntpUDP, "pool.ntp.org", TIME_OFFSET);

void setup() {
  delay(250);

  Serial.begin(115200);
  Wire.begin();
  if (_relay.begin()) {
    Serial.println("Connected to relay.");
  } else {
    Serial.println("RELAY NOT FOUND");
  }
  _relay.turnRelayOff();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PWM_PIN, OUTPUT);
 `
  analogWriteRange(100);
  analogWriteFreq(25000);
  analogWrite(PWM_PIN, 15);

  WiFi.begin("cocacola", "football");
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

      printInfo();
      updateRelay();
    }
  }
}

void blinkLED() {
  bool isWifiConnected = WiFi.status() == WL_CONNECTED;
  long blinkCycleMS = 1000 * (isWifiConnected ? 2 : 0.5);
  digitalWrite(LED_BUILTIN, _currentTime % blinkCycleMS > blinkCycleMS / 2);
}

void updateRelay() {
  int hour = _timeClient.getHours();
  if (hour >= WAKE_TIME && hour < WAKE_TIME + AWAKE_DURATION_HOURS) {
    _relay.turnRelayOn();
    Serial.println("Relay ON");
  } else {
    _relay.turnRelayOff();
    Serial.println("Relay OFF");
  }
  Serial.print("Relay state: ");
  Serial.println(_relay.getState());
}

void printInfo() {
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
