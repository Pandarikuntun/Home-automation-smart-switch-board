#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

/************ CONFIG ************/
#define AP_NAME "ESP32-C3-Setup"
#define AP_PASS "12345678"
/**************switches configure **************/
#define LED_PIN 8

/************ OBJECTS ************/
WebServer server(80);
Preferences prefs;

/************ VARIABLES ************/
String savedSSID = "";
String savedPASS = "";
bool ledState = false;

/************ WIFI CONNECT ************/
bool connectWiFi() {
  if (savedSSID == "") return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  Serial.print("Connecting to WiFi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 15000) return false;
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());
  return true;
}

/************ CONFIG PAGE ************/
String wifiPage() {
  String page =
    "<!DOCTYPE html><html><body>"
    "<h2>WiFi Setup</h2>"
    "<form action='/save'>"
    "WiFi:<br><select name='ssid'>";

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    page += "<option value='" + WiFi.SSID(i) + "'>";
    page += WiFi.SSID(i);
    page += "</option>";
  }

  page +=
    "</select><br><br>"
    "Password:<br>"
    "<input type='password' name='pass'><br><br>"
    "<input type='submit' value='Save & Connect'>"
    "</form></body></html>";

  return page;
}

/************ LED PAGE ************/
String ledPage() {
  return String(
    "<!DOCTYPE html><html><body style='text-align:center;'>"
    "<h2>ESP32-C3 LED Control</h2>"
    "<p>LED is " + String(ledState ? "ON" : "OFF") + "</p>"
    "<button onclick=\"fetch('/on')\">ON</button>"
    "<button onclick=\"fetch('/off')\">OFF</button>"
    "</body></html>"
  );
}

/************ HANDLERS ************/
void handleConfig() {
  server.send(200, "text/html", wifiPage());
}

void handleSave() {
  prefs.begin("wifi", false);
  prefs.putString("ssid", server.arg("ssid"));
  prefs.putString("pass", server.arg("pass"));
  prefs.end();

  server.send(200, "text/html", "<h3>Saved! Restarting...</h3>");
  delay(2000);
  ESP.restart();
}

void handleLED() {
  server.send(200, "text/html", ledPage());
}

void handleON() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  server.send(200, "text/plain", "LED ON");
}

void handleOFF() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", "LED OFF");
}

/************ START CONFIG MODE ************/
void startConfigPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASS);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleConfig);
  server.on("/save", handleSave);
  server.begin();
}

/************ START NORMAL MODE ************/
void startNormalServer() {
  server.on("/", handleLED);
  server.on("/on", handleON);
  server.on("/off", handleOFF);
  server.begin();
}

/************ SETUP ************/
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPASS = prefs.getString("pass", "");
  prefs.end();

  if (connectWiFi()) {
    startNormalServer();
  } else {
    startConfigPortal();
  }
}

/************ LOOP ************/
void loop() {
  server.handleClient();
}
