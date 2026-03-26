#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>
#include <WebServer.h>

#define LED1_PIN 0
#define LED2_PIN 1
#define FAN_PIN  8

#define BTN1_PIN 21
#define BTN2_PIN 20
#define BTN3_PIN 10

#define PWM_FREQ 5000
#define PWM_RES 8

extern WebServer server;

int fanSpeed = 0;

bool led1State = false;
bool led2State = false;
bool fanState = false;

String devicePage() {

  return String(
  "<html><body style='text-align:center;'>"
  "<h2>ESP32 Device Control</h2>"

  "<h3>LED 1</h3>"
  "<button onclick=\"fetch('/on1')\">ON</button>"
  "<button onclick=\"fetch('/off1')\">OFF</button>"

  "<h3>LED 2</h3>"
  "<button onclick=\"fetch('/on2')\">ON</button>"
  "<button onclick=\"fetch('/off2')\">OFF</button>"

  "<h3>FAN</h3>"
  "<input type='range' min='0' max='255' value='" + String(fanSpeed) +
  "' oninput=\"fetch('/fan?val=' + this.value)\">"

  "<br><br><a href='/update'>OTA Update</a>"
  "</body></html>"
  );
}

void handleRoot() {
  server.send(200, "text/html", devicePage());
}

void handleON1() {
  led1State = true;
  digitalWrite(LED1_PIN, HIGH);
}

void handleOFF1() {
  led1State = false;
  digitalWrite(LED1_PIN, LOW);
}

void handleON2() {
  led2State = true;
  digitalWrite(LED2_PIN, HIGH);
}

void handleOFF2() {
  led2State = false;
  digitalWrite(LED2_PIN, LOW);
}

void handleFan() {

  fanSpeed = server.arg("val").toInt();
  ledcWrite(FAN_PIN, fanSpeed);

  if (fanSpeed > 0)
    fanState = true;
  else
    fanState = false;
}

void startDeviceServer() {

  server.on("/", handleRoot);
  server.on("/on1", handleON1);
  server.on("/off1", handleOFF1);
  server.on("/on2", handleON2);
  server.on("/off2", handleOFF2);
  server.on("/fan", handleFan);
}

void initDevices() {

  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BTN3_PIN, INPUT_PULLUP);

  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RES);
}

void checkButtons() {

  static bool lastBtn1 = HIGH;
  static bool lastBtn2 = HIGH;
  static bool lastBtn3 = HIGH;

  bool btn1 = digitalRead(BTN1_PIN);
  bool btn2 = digitalRead(BTN2_PIN);
  bool btn3 = digitalRead(BTN3_PIN);

  if (btn1 == LOW && lastBtn1 == HIGH) {

    led1State = !led1State;
    digitalWrite(LED1_PIN, led1State);
  }

  if (btn2 == LOW && lastBtn2 == HIGH) {

    led2State = !led2State;
    digitalWrite(LED2_PIN, led2State);
  }

  if (btn3 == LOW && lastBtn3 == HIGH) {

    fanState = !fanState;

    if (fanState)
      ledcWrite(FAN_PIN, 255);
    else
      ledcWrite(FAN_PIN, 0);
  }

  lastBtn1 = btn1;
  lastBtn2 = btn2;
  lastBtn3 = btn3;
}

#endif