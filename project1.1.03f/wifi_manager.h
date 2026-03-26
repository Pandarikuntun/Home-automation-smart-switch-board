#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "device_control.h"

#define AP_NAME "ESP32-C3-Setup"
#define AP_PASS "12345678"
#define WIFI_TIMEOUT 15000

WebServer server(80);
Preferences prefs;

String savedSSID, savedPASS;

/************ WIFI PAGE ************/
String wifiPage() {                     //ap mode wifi set up page for select available wifies
  String page =
    "<html><body style='text-align:center;'>"
    "<h2>Select WiFi</h2>"
    "<form action='/save'>"
    "<select name='ssid'>";

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    page += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }

  page +=
    "</select><br><br>"             //pasword place holder for wifi setup page in ap mode
    "<input type='password' name='pass' placeholder='Password'><br><br>"
    "<input type='submit' value='Connect'>"
    "</form></body></html>";

  return page;
}

void handleConfig() {
  server.send(200, "text/html", wifiPage());
}

void handleSave() {

  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();

  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.print("Connecting to WiFi");

  int timeout = 0;

  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {

    String ip = WiFi.localIP().toString();

    String page =                 //ofter saving wifi credentials show ip addres of device page that ip is local ip address
      "<html><body style='text-align:center;font-family:Arial;'>"
      "<h2>WiFi Connected</h2>"
      "<p>Device Control Page:</p>"
      "<h3>http://" + ip + "</h3>"
      "<br><br>"
      "<a href='http://" + ip + "'>"
      "<button style='padding:10px 20px;font-size:16px;'>Open Control Page</button>"
      "</a>"
      "</body></html>";

    server.send(200, "text/html", page);
  }
  else {

    server.send(200, "text/html",
    "<h3>WiFi Connection Failed. Restart ESP32.</h3>");
  }
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

/************ AP MODE ********/
void startAP() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASS);

  Serial.println("📡 AP MODE");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleConfig);
  server.on("/save", handleSave);
  server.onNotFound(handleNotFound);
  server.begin();
}

/******************** CONNECT WIFI ************/
bool connectWiFi() {
  if (savedSSID == "") return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT) return false;
    delay(500);
  }

  Serial.print(" Connected: ");
  Serial.println(WiFi.localIP());

  startDeviceServer();
  return true;
}

/************************ INIT WIFI ************/
void initWiFi() {
  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPASS = prefs.getString("pass", "");
  prefs.end();

  if (!connectWiFi()) startAP();
}
