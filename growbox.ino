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

// i2c and PWM appear to compete, so we can only do one job at a time.
typedef enum {
  // Lighting job uses a rely via i2c to control the lights.
  LIGHT_JOB,
  // Fan job uses PWM to control fan speed.
  FAN_JOB
} job_t;

job_t _job = LIGHT_JOB;

unsigned long _currentTime = 0;
unsigned long _blinkTime = 0;
unsigned long _ntpSyncTime = 0;
bool _wifi_connected = false;
Qwiic_Relay _relay(RELAY_ADDR);
WiFiUDP _ntpUDP;
NTPClient _timeClient(_ntpUDP, "pool.ntp.org", TIME_OFFSET);

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, true);

  delay(250);
  Serial.println("********");

  if (_job == LIGHT_JOB) {
    Wire.begin();
    if (_relay.begin()) {
      Serial.println("Connected to relay.");
    } else {
      Serial.println("RELAY NOT FOUND");
    }
    _relay.turnRelayOff();
  }

  if (_job == FAN_JOB) {
    pinMode(PWM_PIN, OUTPUT);
    analogWriteRange(100);
    analogWriteFreq(25000);
    analogWrite(PWM_PIN, 15);
  }

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
      if (_job == LIGHT_JOB) {
        updateRelay();
      }
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
  const float nominalDivisor = nominalDutyCycle * 2;

  float dutyCycle = nominalDutyCycle;
  float divisor = nominalDivisor;
  if (WiFi.status() != WL_CONNECTED) {
    dutyCycle = wifiErrorDutyCycle;
    divisor = errorDivisor;
  }
  if (_job == LIGHT_JOB && _relay.getState() == 255) {
    // Sometimes this sparkfun qwiic relay returns 255 for its state, which indicates the
    // relay is not connected or malfunctioning. e.g. if you've changed the PWM settings :)
    dutyCycle = relayErrorDutyCycle;
    divisor = errorDivisor;
  }

  long blinkCycleMS = 1000 * dutyCycle;
  digitalWrite(LED_BUILTIN, _currentTime % blinkCycleMS > blinkCycleMS / divisor);
}

void updateRelay() {
  int hour = _timeClient.getHours();
  int sleepTime = WAKE_TIME + AWAKE_DURATION_HOURS;
  int relayState = _relay.getState();
  
  Serial.print("wake time: ");
  Serial.print(WAKE_TIME);
  Serial.print(", sleep time: ");
  Serial.print(sleepTime);
  Serial.print(", current time: ");
  Serial.println(hour);
  
  if (hour >= WAKE_TIME && hour < sleepTime) {
    if (relayState != 1) {
      _relay.turnRelayOn();
    }
    Serial.println("Turn relay ON");
  } else {
    if (relayState != 0) {
      _relay.turnRelayOff();
    }
    Serial.println("Turn relay OFF");
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
