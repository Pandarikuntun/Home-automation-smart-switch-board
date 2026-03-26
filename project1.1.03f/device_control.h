#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>
#include <WebServer.h>

/* ----- SAFE GPIO PINS FOR ESP32-C3 -------- */

#define LED1_PIN 2
#define LED2_PIN 3
#define FAN_PIN  4

#define BTN1_PIN 10
#define BTN2_PIN 9
#define BTN3_PIN 8

/* ---- PWM CONFIG -------- */

#define PWM_FREQ 5000
#define PWM_RES  8

/* ---- CURRENT SENSOR -------- */

const int SENSOR = 0;
const float ADC_ref = 3.3;
const float ADC_res = 4095.0;

float sensitivity = 185.0;
float offsetvolt = 1.65;

/* ------ SERVER -------- */

extern WebServer server;

/* ------ STATES -------- */

int fanSpeed = 0;

bool led1State = false;
bool led2State = false;
bool fanState  = false;

bool lastBtn1 = HIGH;
bool lastBtn2 = HIGH;
bool lastBtn3 = HIGH;

/* -------- WEB PAGE ---- */

String devicePage() {

  return String(
  "<html><body style='text-align:center;font-family:Arial;'>"

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

/* ---- WEB HANDLERS -------- */

void handleRoot() { 
  server.send(200, "text/html", devicePage()); 
}

void handleON1() { 
  digitalWrite(LED1_PIN, HIGH); 
}

void handleOFF1(){ 
  digitalWrite(LED1_PIN, LOW); 
}

void handleON2() { 
  digitalWrite(LED2_PIN, HIGH); 
}

void handleOFF2(){ 
  digitalWrite(LED2_PIN, LOW); 
}

void handleFan() {

  fanSpeed = server.arg("val").toInt();

  ledcWrite(FAN_PIN, fanSpeed);

}

/* ---- START WEB SERVER -------- */

void startDeviceServer() {

  server.on("/", handleRoot);

  server.on("/on1", handleON1);
  server.on("/off1", handleOFF1);

  server.on("/on2", handleON2);
  server.on("/off2", handleOFF2);

  server.on("/fan", handleFan);

  server.begin();

}

/* --- INIT DEVICES -------- */

void initDevices() {

  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);

  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED2_PIN, LOW);

  /* PWM FAN SETUP */

  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RES);
  ledcWrite(FAN_PIN, 0);

  /* BUTTON INPUTS */

  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);
  pinMode(BTN3_PIN, INPUT_PULLUP);

  /* SENSOR pin mode*/

  pinMode(SENSOR, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
}

/* -- CURRENT SENSOR -------- */

void current_sensor(){

  float avgAdc = 0;
  int samples = 50;

  for(int i=0;i<samples;i++){
    avgAdc += analogRead(SENSOR);
  }

  avgAdc /= samples;

  float voltage = (avgAdc * ADC_ref) / ADC_res;

  float current = ((voltage - offsetvolt) * 1000) / sensitivity;

  Serial.print("Current: ");
  Serial.println(current);

  if(current > 15){       /////////amps change required as per the require

    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);

    ledcWrite(FAN_PIN, 0);

  }

}

/* ---- BUTTON CONTROL -------- */

void handleButtons() {

  bool btn1 = digitalRead(BTN1_PIN);
  bool btn2 = digitalRead(BTN2_PIN);
  bool btn3 = digitalRead(BTN3_PIN);

  /* BUTTON 1 -> LED1 */

  if (lastBtn1 == HIGH && btn1 == LOW) {

    led1State = !led1State;

    digitalWrite(LED1_PIN, led1State ? HIGH : LOW);

    delay(200);
  }

  /* BUTTON 2 -> LED2 */

  if (lastBtn2 == HIGH && btn2 == LOW) {

    led2State = !led2State;

    digitalWrite(LED2_PIN, led2State ? HIGH : LOW);

    delay(200);
  }

  /* BUTTON 3 -> FAN */

  if (lastBtn3 == HIGH && btn3 == LOW) {

    fanState = !fanState;

    ledcWrite(FAN_PIN, fanState ? 255 : 0);

    delay(200);
  }

  lastBtn1 = btn1;
  lastBtn2 = btn2;
  lastBtn3 = btn3;

}

#endif