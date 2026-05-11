#ifndef DEVICE_CONTROL_H
#define DEVICE_CONTROL_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebServer.h>

extern WebServer server;

/************ FIREBASE url************/
#define FIREBASE_HOST "https://esp32-c3-70289-default-rtdb.firebaseio.com/"

WiFiClientSecure client;

/************ PINS delarare ************/
#define OUT1_PIN 5        //BTN1_PIN 6
#define OUT2_PIN 2        //BTN1_PIN 7
#define OUT3_PIN 3        //BTN1_PIN 8
#define OUT4_PIN 4        //BTN1_PIN 9

#define BTN1_PIN 6
#define BTN2_PIN 7
#define BTN3_PIN 8   
#define BTN4_PIN 9

#define FAN_PIN  10       //BTNFAN_PIN 21
#define BTNFAN_PIN 21

#define TOTAL_OUTPUTS 4

#define PWM_FREQ 5000
#define PWM_RES  8

/************ STATES ************/
bool outState[TOTAL_OUTPUTS] = {0};
bool lastBtn[TOTAL_OUTPUTS] = {HIGH,HIGH,HIGH,HIGH};

bool lastFanBtn = HIGH;
int fanStep = 0;

bool needUpdate[TOTAL_OUTPUTS] = {0};   
bool fanNeedUpdate = false;             
int fanValueToSend = 0;                 

#define LOCAL_CONTROL_DELAY 1000   // .2 sec

/****************current sensor defining***************/

#define SENSOR_PIN 0        //  change if needed (ADC pin)
#define ADC_REF 3.3
#define ADC_RES 4095
float sensitivity = 185.0;   // ACS712 5A = 185 mV/A
float offsetVoltage = 1.65;
#define CURRENT_LIMIT 2.0    //  2 Amps limit change here
bool overloadActive = false;

/*queue for update */
struct UpdateQueue {
  int index;
  bool value;
};
UpdateQueue queue[10];
int queueSize = 0;

bool localControlActive = false;
unsigned long localControlTime = 0;

/************ HTML PAGE ************/
String devicePage() {
    return String(R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
    <title>ESP32 Smart Control Panel</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
    body{font-family:Arial;margin:0;background:#0f172a;color:white;text-align:center;}
    h2{margin-top:20px;color:#38bdf8;}
    .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px;padding:15px;}
    .card{background:#1e293b;padding:15px;border-radius:12px;}
    button{padding:8px;margin:5px;border:none;border-radius:8px;font-weight:bold;}
    .on{background:#22c55e;color:white;}
    .off{background:#ef4444;color:white;}
    .slider-box{background:#1e293b;margin:10px;padding:15px;border-radius:12px;}
    </style>
    </head>

    <body>

    <h2> ESP32 Smart Control Panel</h2>

    <div class="grid" id="buttons"></div>

    <div class="slider-box">
    <h3>🌬 FAN Control</h3>
    <input type="range" min="0" max="255" value="0" oninput="fan(this.value)">
    <p id="fanval">0%</p>
    </div>

    <script type="module">

    import { initializeApp } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-app.js";
    import { getDatabase, ref, set } from "https://www.gstatic.com/firebasejs/10.7.1/firebase-database.js";

    const firebaseConfig = {
        apiKey: "AIzaSyCfMFnVHJHUp55aVb1WNT8kQCyWjypiwJ8",
        authDomain: "70289.firebaseapp.com",
        databaseURL: "https://70289-default-rtdb.firebaseio.com",
        projectId: "70289",
        storageBucket: "70289.firebasestorage.app",
        messagingSenderId: "951645057066",
        appId: "1:951645057066:web:7a650a5d4d0215f5b8d353",
      
    };

    const app = initializeApp(firebaseConfig);
    const db = getDatabase(app);

    const container = document.getElementById("buttons");

    for (let i = 1; i <= 4; i++) {
    container.innerHTML += `
    <div class="card">
    <h3>OUT${i}</h3>
    <button class="on" onclick="on(${i})">ON</button>
    <button class="off" onclick="off(${i})">OFF</button>
    </div>`;
    }

    window.on = (n)=>{ set(ref(db,"device/out"+n),true); }
    window.off = (n)=>{ set(ref(db,"device/out"+n),false); }

    window.fan = (val)=>{
    document.getElementById("fanval").innerText = val+"%";
    set(ref(db,"device/fan1"),Number(val));
    }

    </script>

    </body>
    </html>
)rawliteral");
}

/************ FIREBASE ************/

bool firebaseGet(String path) {
  if (WiFi.status() != WL_CONNECTED) return false;
  //client.setInsecure();
  HTTPClient https;
  String url = String(FIREBASE_HOST) + path + ".json";
  https.begin(client, url);
  https.setReuse(true);   
  int code = https.GET();
  bool val = false;

  if (code == 200) {
    val = (https.getString() == "true"); 
  }

  https.end();
  return val;
}
/*-------------------set---------*/
void firebaseSet(String path, bool value) {
  //client.setInsecure();
  HTTPClient https;
  String url = String(FIREBASE_HOST) + path + ".json";
  https.begin(client, url);
  https.setReuse(true);   
  https.addHeader("Content-Type", "application/json");
  https.PUT(value ? "true" : "false");
  https.end();
}
void firebaseSetInt(String path, int value) {
  //client.setInsecure();
  HTTPClient https;
  String url = String(FIREBASE_HOST) + path + ".json";
  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");
  https.PUT(String(value));   //  send number
  https.end();
}

/************ FIREBASE SYNC ************/
void firebaseReadFast() {

  if (WiFi.status() != WL_CONNECTED) return;

  //  BLOCK read during local control
  if (localControlActive && millis() - localControlTime < LOCAL_CONTROL_DELAY) {
    return;
  }

  localControlActive = false;

  //  SKIP if queue pending
  if (queueSize > 0 || fanNeedUpdate) return;

  static unsigned long lastRead = 0;
  if (millis() - lastRead < 1500) return;
  lastRead = millis();
  int pins[TOTAL_OUTPUTS] = {OUT1_PIN, OUT2_PIN, OUT3_PIN, OUT4_PIN};

  for (int i = 0; i < TOTAL_OUTPUTS; i++) {
    bool v = firebaseGet("/device/out" + String(i+1));

    if (v != outState[i]) {
      outState[i] = v;
      digitalWrite(pins[i], v);
    }
  }

  //  FAN READ
  HTTPClient https;
  //client.setInsecure();
  String url = String(FIREBASE_HOST) + "/device/fan1.json";
  https.begin(client, url);
  int code = https.GET();
  if (code == 200) {
    int val = https.getString().toInt();
    ledcWrite(FAN_PIN, val);
  }

  https.end();
}
/***********firebase update**********/
void sendFirebaseUpdates() {

  if (WiFi.status() != WL_CONNECTED) return;

  static unsigned long lastSend = 0;
  if (millis() - lastSend < 100) return;
  lastSend = millis();

  //  OUTPUT QUEUE
  if (queueSize > 0) {
    UpdateQueue item = queue[0];
    for (int i = 0; i < queueSize - 1; i++) {
      queue[i] = queue[i + 1];
    }
    queueSize--;
    firebaseSet("/device/out" + String(item.index + 1), item.value);
    return;
  }

  //  FAN UPDATE
  if (fanNeedUpdate) {
    firebaseSetInt("/device/fan1", fanValueToSend);
    fanNeedUpdate = false;
  }
}
/************ BUTTON CONTROL ************/
void handleButtons() {

  if (overloadActive) return;

  int btnPins[TOTAL_OUTPUTS] = {BTN1_PIN, BTN2_PIN, BTN3_PIN, BTN4_PIN};
  int outPins[TOTAL_OUTPUTS] = {OUT1_PIN, OUT2_PIN, OUT3_PIN, OUT4_PIN};

  static unsigned long lastPress[TOTAL_OUTPUTS] = {0};
  for (int i = 0; i < TOTAL_OUTPUTS; i++) {
    bool cur = digitalRead(btnPins[i]);

    if (lastBtn[i] == HIGH && cur == LOW && millis() - lastPress[i] > 50) {
      lastPress[i] = millis();

      //  INSTANT OUTPUT
      outState[i] = !outState[i];
      digitalWrite(outPins[i], outState[i]);

      //  LOCAL PRIORITY
      localControlActive = true;
      localControlTime = millis();

      //  ADD TO QUEUE
      if (queueSize < 10) {
        queue[queueSize++] = {i, outState[i]};
      }
    }

    lastBtn[i] = cur;
  }
}
/************ FAN BUTTON ************/
void handleFanButton() {

  if (overloadActive) return;

  int levels[5] = {0, 64, 128, 192, 255};
  static unsigned long lastPress = 0;
  bool cur = digitalRead(BTNFAN_PIN);

  if (lastFanBtn == HIGH && cur == LOW && millis() - lastPress > 10) {

    lastPress = millis();
    fanStep = (fanStep + 1) % 5;
    int val = levels[fanStep];

    //  INSTANT
    static int lastFanVal = -1;

    if (val != lastFanVal) {
        ledcWrite(FAN_PIN, val);
        lastFanVal = val;
}

    localControlActive = true;
    localControlTime = millis();

    //  QUEUE
    fanNeedUpdate = true;
    fanValueToSend = val;
  }
  lastFanBtn = cur;
}
/************ SERVER ************/
void handleRoot(){
  server.send(200,"text/html",devicePage());
}
void startDeviceServer(){
  server.on("/",handleRoot);
  server.begin();
}

/************ INIT ************/
void initDevices(){
  int outPins[TOTAL_OUTPUTS]={OUT1_PIN,OUT2_PIN,OUT3_PIN,OUT4_PIN};
  int btnPins[TOTAL_OUTPUTS]={BTN1_PIN,BTN2_PIN,BTN3_PIN,BTN4_PIN};
  for(int i=0;i<TOTAL_OUTPUTS;i++){
    pinMode(outPins[i],OUTPUT);
    pinMode(btnPins[i],INPUT_PULLUP);
  }
  pinMode(BTNFAN_PIN, INPUT_PULLUP);
  ledcAttach(FAN_PIN, PWM_FREQ, PWM_RES);
}
/*********overloade check********/

void checkCurrent() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 200) return;   
  lastCheck = millis();

  float avgAdc = 0;
  int samples = 5;
  for (int i = 0; i < samples; i++) {
    avgAdc += analogRead(SENSOR_PIN);
  }
  avgAdc /= samples;
  float voltage = (avgAdc * ADC_REF) / ADC_RES;
  float current = ((voltage - offsetVoltage) * 1000) / sensitivity;

  if (current > CURRENT_LIMIT) {

    if (!overloadActive) {   
      overloadActive = true;
      // OUTPUTS oFF
      int pins[TOTAL_OUTPUTS] = {OUT1_PIN, OUT2_PIN, OUT3_PIN, OUT4_PIN};

      for (int i = 0; i < TOTAL_OUTPUTS; i++) {
        outState[i] = false;
        digitalWrite(pins[i], LOW);

        if (WiFi.status() == WL_CONNECTED) {
          firebaseSet("/device/out" + String(i+1), false);
        }
      }
      // FAN off
      ledcWrite(FAN_PIN, 0);

      if (WiFi.status() == WL_CONNECTED) {
        firebaseSetInt("/device/fan1", 0);
      }
    }
  }
  else {
    overloadActive = false;   // safe for now 
  }
}
#endif
