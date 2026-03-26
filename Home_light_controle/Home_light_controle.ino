/************ LIBRARIES ************/
#include <ESP8266WiFi.h>      // wifi library
#include <ESP8266WebServer.h> //web server library
#include <SinricPro.h>    //sinricpro cloud library voice controle
#include <SinricProSwitch.h>

/************ WIFI ************/
#define WIFI_SSID "Redmi 9 Power"   //ssid
#define WIFI_PASS "TULASIRAM"     //pasword of wifi

/************ SINRIC PRO ************/
#define APP_KEY    "369a87ad-eea2-4275-8e0e-5ec89897c28c"     //sinricpro appkey
#define APP_SECRET "9c31a0dc-38bb-407d-b569-ed4ac049a605-f1cbff0c-a08c-466c-8ee2-da36ae752a1b"      // sinricpro app secret
#define DEVICE_ID  "694cd1436ebb39d664d3efc3"       ////devise id for switch or voice controle

/************ PINS ************/
#define LED_PIN     5           //D1 led connected GPIO_5
#define BUTTON_PIN  4           //D2 button connected GPIO_4
#define IR_PIN      16          //D0 ir connected GPIO_16

ESP8266WebServer server(80);      //creat webserver on port number 80

/************ VARIABLES ************/
bool webLedState = false;          //stores ON/OFF ststus of app from web
int brightness = 512;               //default brightnes at 50 persentage
String irStatus = "Not Detected";   //IR ststus
String ledStatus = "OFF";       //LED ststus text

/************ SINRIC CALLBACK ************/
bool onPowerState(const String &, bool &state) {      //this function workes on speech command from webserver 
  webLedState = state;                                // stores ON/OFF state of led
  if (state) {                            //if on LED ON with selected brightnes
    analogWrite(LED_PIN, brightness);
    ledStatus = "ON";
  } else {                                // if Switch off LED OFF
    digitalWrite(LED_PIN, LOW);
    ledStatus = "OFF";
  }
  return true;            //Conformation from sincripro
}

/************ WEB PAGE ************/
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP8266 Smart LED</title>

<style>
body {
  background: linear-gradient(135deg,#667eea,#764ba2);
  font-family: 'Segoe UI', sans-serif;
  text-align: center;
  color: white;
  margin: 0;
  padding: 0;
}

.card {
  background: white;
  color: #333;
  max-width: 350px;
  margin: 40px auto;
  padding: 25px;
  border-radius: 15px;
  box-shadow: 0 10px 25px rgba(0,0,0,0.3);
}

h2 { margin-bottom: 10px; }

.led {
  width: 60px;
  height: 60px;
  border-radius: 50%;
  margin: 15px auto;
  background: black;
  box-shadow: 0 0 10px red;
}

.on {
  background: #2ecc71;
  box-shadow: 0 0 20px #2ecc71;
}

button {
  width: 120px;
  padding: 12px;
  margin: 10px;
  font-size: 16px;
  border: none;
  border-radius: 8px;
  color: white;
  cursor: pointer;
}

#onBtn { background: #27ae60; }
#offBtn { background: #e74c3c; }

input[type=range] {
  width: 100%;
  margin-top: 10px;
}

.status {
  font-weight: bold;
}
</style>
</head>

<body>

<div class="card">
  <h2>HOME Smart LED</h2>

  <div id="ledIcon" class="led"></div>

  <p>LED: <span id="led" class="status">OFF</span></p>
  <p>IR Sensor: <span id="ir" class="status">Not Detected</span></p>

  <button id="onBtn" onclick="fetch('/on')">ON</button>
  <button id="offBtn" onclick="fetch('/off')">OFF</button>

  <p>Brightness: <span id="val">50</span></p>
  <input type="range" min="0" max="100" value="50" oninput="updateBrightness(this.value)">
</div>

<script>
function updateBrightness(v){
  document.getElementById("val").innerHTML = v;
  fetch('/slider?value=' + v);
}

setInterval(() => {
  fetch('/ledstatus').then(r=>r.text()).then(d=>{
    document.getElementById("led").innerHTML = d;
    document.getElementById("ledIcon").className =
      d === "ON" ? "led on" : "led";
  });

  fetch('/irstatus').then(r=>r.text()).then(d=>{
    document.getElementById("ir").innerHTML = d;
  });
}, 500);
</script>

</body>
</html>
)rawliteral";
    // send webpage to browser
  server.send(200, "text/html", html);
}

/************ SETUP ************/
void setup() {
  Serial.begin(9600);     //burd rate for for the data transfer serial monitor
/* --------cofigure pins----------*/
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(IR_PIN, INPUT);

  analogWriteRange(100);            //brightnes range 0-100
  digitalWrite(LED_PIN, LOW);         //initially off state

  WiFi.begin(WIFI_SSID, WIFI_PASS);     //connect to wifi
  while (WiFi.status() != WL_CONNECTED) delay(500);

  // prints  IP ADDRESS in serial window
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);     ///web server working

  server.on("/on", []() {
    webLedState = true;
    analogWrite(LED_PIN, brightness);
    ledStatus = "ON";
    server.send(200, "text/plain", "ON");
  });

  server.on("/off", []() {
    webLedState = false;
    digitalWrite(LED_PIN, LOW);
    ledStatus = "OFF";
    server.send(200, "text/plain", "OFF");
  });

  server.on("/slider", []() {
    if (server.hasArg("value")) {
      brightness = server.arg("value").toInt();
      webLedState = true;
      analogWrite(LED_PIN, brightness);
      ledStatus = "ON";
    }
    server.send(200, "text/plain", "OK");
  });
      /** status end point **/
  server.on("/irstatus", []() { server.send(200,"text/plain",irStatus); });
  server.on("/ledstatus", []() { server.send(200,"text/plain",ledStatus); });

  server.begin();     //web server starts
/*              sinricpro setup           */
  SinricProSwitch &sw = SinricPro[DEVICE_ID];
  sw.onPowerState(onPowerState);
  SinricPro.begin(APP_KEY, APP_SECRET);
}

/************ LOOP ************/
void loop() {
  server.handleClient();      //web request
  SinricPro.handle();         //sunricpro request

  bool btn = digitalRead(BUTTON_PIN);   //button
  bool ir  = digitalRead(IR_PIN);     //ir sensor

  irStatus = (ir == LOW) ? "Detected" : "Not Detected";       //IR status display web
 
 //   If button OR IR OR web command → LED ON        //
  
  if (btn == HIGH || ir == LOW || webLedState) {
    analogWrite(LED_PIN, brightness);
    ledStatus = "ON";
  } else {
    digitalWrite(LED_PIN, LOW);
    ledStatus = "OFF";
  }
}
