#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>
#include <WebServer.h>

#define LED1_PIN 8
#define LED2_PIN 5
#define FAN_PIN  6

#define SW_LED1  21     //switch
#define SW_LED2  20

#define PWM_FREQ 5000
#define PWM_RES  8

bool led1State = false;
bool led2State = false;

unsigned long lastDebounce1 = 0;
unsigned long lastDebounce2 = 0;
const unsigned long debounceDelay = 200;

extern WebServer server;

int fanSpeed = 0;

String devicePage() {
  return String(
    "<html><body style='text-align:center;'>"
    "<h2>ESP32 Device Control</h2>"
    "<button onclick=\"fetch('/on1')\">LED1 ON</button>"
    "<button onclick=\"fetch('/off1')\">LED1 OFF</button><br><br>"
    "<button onclick=\"fetch('/on2')\">LED2 ON</button>"
    "<button onclick=\"fetch('/off2')\">LED2 OFF</button><br><br>"
    "<h3>FAN SPEED</h3>"
    "<input type='range' min='0' max='255' value='0' "
    "oninput=\"fetch('/fan?val='+this.value)\">"
    "</body></html>"
  );
}

void handleRoot() { server.send(200, "text/html", devicePage()); }
void handleON1() { digitalWrite(LED1_PIN, HIGH); }
void handleOFF1(){ digitalWrite(LED1_PIN, LOW); }
void handleON2() { digitalWrite(LED2_PIN, HIGH); }
void handleOFF2(){ digitalWrite(LED2_PIN, LOW); }

void handleFan() {
  fanSpeed = server.arg("val").toInt();
  ledcWrite(FAN_PIN, fanSpeed);
}

void startDeviceServer() {
  server.on("/", handleRoot);
  server.on("/on1", handleON1);
  server.on("/off1", handleOFF1);
  server.on("/on2", handleON2);
  server.on("/off2", handleOFF2);
  server.on("/fan", handleFan);
  server.begin();
}

void initDevices() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
    
  pinMode(SW_LED1, INPUT_PULLUP);
  pinMode(SW_LED2, INPUT_PULLUP);

  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RES);
}
void handleSwitches() {

  if (digitalRead(SW_LED1) == LOW && millis() - lastDebounce1 > debounceDelay) {
    led1State = !led1State;
    digitalWrite(LED1_PIN, led1State);
    lastDebounce1 = millis();
  }

  if (digitalRead(SW_LED2) == LOW && millis() - lastDebounce2 > debounceDelay) {
    led2State = !led2State;
    digitalWrite(LED2_PIN, led2State);
    lastDebounce2 = millis();
  }
}


#endif
