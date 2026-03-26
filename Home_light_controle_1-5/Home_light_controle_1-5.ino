/************ LIBRARIES ************/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SinricPro.h>
#include <SinricProLight.h>

/************ WIFI ************/
#define WIFI_SSID "Redmi 9 Power"
#define WIFI_PASS "TULASIRAM"

/************ SINRIC PRO ************/
#define APP_KEY    "369a87ad-eea2-4275-8e0e-5ec89897c28c"
#define APP_SECRET "9c31a0dc-38bb-407d-b569-ed4ac049a605-f1cbff0c-a08c-466c-8ee2-da36ae752a1b"
#define DEVICE_ID  "694ced0c00f870dd77d52a7f"

/************ PINS ************/
#define LED_PIN     5     // D1
#define BUTTON_PIN  4     // D2
#define IR_PIN      16    // D0

ESP8266WebServer server(80);

/************ VARIABLES ************/
bool ledState = false;
int brightness = 50;          // 0–100
String irStatus = "Not Detected";
String ledStatus = "OFF";

/************ SINRIC CALLBACKS ************/
bool onPowerState(const String &, bool &state) {
  ledState = state;
  if (!state) {
    digitalWrite(LED_PIN, LOW);
    ledStatus = "OFF";
  }
  return true;
}

bool onBrightness(const String &, int &level) {
  brightness = constrain(level, 0, 100);
  ledState = true;            // speaking brightness turns ON
  if (level <= 20)       brightness = 20;
  else if (level <= 40)  brightness = 40;
  else if (level <= 60)  brightness = 60;
  else if (level <= 80)  brightness = 80;
  else                   brightness = 100;
  analogWrite(LED_PIN, brightness);
  ledStatus = "ON";
  Serial.printf("VOICE BRIGHTNESS: %d%%\n", brightness);
  return true;
}

/************ WEB PAGE ************/
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP8266 Smart LED</title>
</head>
<body style="text-align:center;font-family:Arial">
<h2>SMART LED</h2>
<p>LED: <span id="led">OFF</span></p>
<p>IR: <span id="ir">Not Detected</span></p>
<button onclick="fetch('/on')">ON</button>
<button onclick="fetch('/off')">OFF</button>
<p>Brightness: <span id="val">50</span></p>
<input type="range" min="0" max="100" value="50"
 oninput="setB(this.value)">
<script>
function setB(v){
 document.getElementById("val").innerHTML=v;
 fetch('/slider?value='+v);
}
setInterval(()=>{
 fetch('/ledstatus').then(r=>r.text()).then(d=>{
  document.getElementById("led").innerHTML=d;
 });
 fetch('/irstatus').then(r=>r.text()).then(d=>{
  document.getElementById("ir").innerHTML=d;
 });
},500);
</script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}

/************ SETUP ************/
void setup() {
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(IR_PIN, INPUT);

  analogWriteRange(100);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/on", [](){
    ledState = true;
    analogWrite(LED_PIN, brightness);
    ledStatus = "ON";
    server.send(200,"text/plain","ON");
  });

  server.on("/off", [](){
    ledState = false;
    digitalWrite(LED_PIN, LOW);
    ledStatus = "OFF";
    server.send(200,"text/plain","OFF");
  });

  server.on("/slider", [](){
    brightness = server.arg("value").toInt();
    ledState = true;
    analogWrite(LED_PIN, brightness);
    ledStatus = "ON";
    server.send(200,"text/plain","OK");
  });

  server.on("/irstatus", [](){ server.send(200,"text/plain",irStatus); });
  server.on("/ledstatus", [](){ server.send(200,"text/plain",ledStatus); });

  server.begin();

  /************ SINRIC SETUP ************/
  SinricProLight &light = SinricPro[DEVICE_ID];
  light.onPowerState(onPowerState);
  light.onBrightness(onBrightness);
  SinricPro.begin(APP_KEY, APP_SECRET);
}

/************ LOOP ************/
void loop() {
  server.handleClient();
  SinricPro.handle();

  bool btn = digitalRead(BUTTON_PIN) == HIGH;
  bool ir  = digitalRead(IR_PIN) == LOW;

  irStatus = ir ? "Detected" : "Not Detected";

  if (btn || ir || ledState) {
    analogWrite(LED_PIN, brightness);
    ledStatus = "ON";
  } else {
    digitalWrite(LED_PIN, LOW);
    ledStatus = "OFF";
  }
}
