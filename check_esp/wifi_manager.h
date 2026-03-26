#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include "device_control.h"

#define AP_NAME "ESP32-C3-Setup"
#define AP_PASS "123456789"
#define WIFI_TIMEOUT 15000

WebServer server(80);
Preferences prefs;

String savedSSID, savedPASS;

// ---------------- CONNECT WIFI ----------------
bool connectWiFi() {
  if (savedSSID == "") return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  unsigned long start = millis();
  Serial.print("🔄 Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > WIFI_TIMEOUT) {
      Serial.println("\n❌ Failed");
      return false;
    }
  }

  Serial.println("\n✅ WiFi Connected");
  Serial.print("🌐 Control Page: http://");
  Serial.println(WiFi.localIP());

  startDeviceServer();
  return true;
}

// ---------------- WIFI PAGE ----------------
String wifiPage() {
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
    "</select><br><br>"
    "<input type='password' name='pass' placeholder='Password'><br><br>"
    "<input type='submit' value='Connect'>"
    "</form></body></html>";

  return page;
}

void handleConfig() {
  server.send(200, "text/html", wifiPage());
}

void handleSave() {
  prefs.begin("wifi", false);
  prefs.putString("ssid", server.arg("ssid"));
  prefs.putString("pass", server.arg("pass"));
  prefs.end();

  server.send(200, "text/html",
    "<h3>Saved! Rebooting...</h3>");
  delay(2000);
  ESP.restart();
}

// Captive portal redirect
void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// ---------------- AP MODE ----------------
void startAP() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASS);

  Serial.println("📡 AP MODE STARTED");
  Serial.print("📡 Connect to WiFi: ");
  Serial.println(AP_NAME);

  server.on("/", handleConfig);
  server.on("/save", handleSave);
  server.onNotFound(handleNotFound);
  server.begin();
}

// ---------------- INIT ----------------
void initWiFi() {
  Serial.println("initWiFi() called");

  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPASS = prefs.getString("pass", "");
  prefs.end();

  if (!connectWiFi()) {
    startAP();
  }
}

#endif
