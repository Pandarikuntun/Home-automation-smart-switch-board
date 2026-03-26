#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

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

bool connectWiFi() {

  if (savedSSID == "") return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  unsigned long start = millis();

  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");

    if (millis() - start > WIFI_TIMEOUT) {
      Serial.println("\nWiFi Failed");
      return false;
    }
  }

  Serial.println("\nWiFi Connected");
  Serial.print("Device Page: http://");
  Serial.println(WiFi.localIP());

  startDeviceServer();
  server.begin();

  return true;
}

String wifiPage() {

  String page =
  "<html><body style='text-align:center;'>"
  "<h2>Select WiFi Network</h2>"
  "<form action='/save'>"
  "<select name='ssid'>";

  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; i++) {

    page += "<option value='" + WiFi.SSID(i) + "'>";
    page += WiFi.SSID(i);
    page += "</option>";
  }

  page +=
  "</select><br><br>"
  "<input type='password' name='pass' placeholder='WiFi Password'><br><br>"
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

  savedSSID = ssid;
  savedPASS = pass;

  server.send(200, "text/html", "<h3>Connecting to WiFi...</h3>");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    if (millis() - start > WIFI_TIMEOUT) {

      server.send(200, "text/html",
      "<html><body style='text-align:center;'>"
      "<h2 style='color:red;'>WiFi Connection Failed</h2>"
      "<a href='/'>Try Again</a>"
      "</body></html>");

      return;
    }
  }

  startDeviceServer();
  server.begin();

  String ip = WiFi.localIP().toString();

  String successPage =
  "<html><body style='text-align:center;'>"
  "<h2 style='color:green;'>WiFi Connected</h2>"
  "<h3>Device Control Page</h3>"
  "<a href='http://" + ip + "'>"
  "http://" + ip + "</a>"
  "<br><br>"
  "<h3>OTA Update Page</h3>"
  "<a href='http://" + ip + "/update'>"
  "http://" + ip + "/update</a>"
  "</body></html>";

  server.send(200, "text/html", successPage);
}

void handleNotFound() {

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void startAP() {

  WiFi.disconnect(true);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASS);

  Serial.println("AP Started");
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleConfig);
  server.on("/save", handleSave);
  server.onNotFound(handleNotFound);

  server.begin();
}

void initWiFi() {

  prefs.begin("wifi", true);
  savedSSID = prefs.getString("ssid", "");
  savedPASS = prefs.getString("pass", "");
  prefs.end();

  if (!connectWiFi()) {
    startAP();
  }
}

#endif