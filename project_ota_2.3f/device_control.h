#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>
#include <WebServer.h>

#define LED1_PIN 8
#define LED2_PIN 5
#define FAN_PIN  6

#define PWM_FREQ 5000
#define PWM_RES  8

extern WebServer server;

int fanSpeed = 0;

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

void handleRoot() { server.send(200, "text/html", devicePage()); }
void handleON1() { digitalWrite(LED1_PIN, HIGH); }
void handleOFF1() { digitalWrite(LED1_PIN, LOW); }
void handleON2() { digitalWrite(LED2_PIN, HIGH); }
void handleOFF2() { digitalWrite(LED2_PIN, LOW); }
void handleFan() { fanSpeed = server.arg("val").toInt();
  ledcWrite(FAN_PIN, fanSpeed);
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
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RES);
}

#endif
