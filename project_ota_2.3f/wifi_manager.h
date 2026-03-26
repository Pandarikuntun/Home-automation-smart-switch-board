#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
//#include <Update.h>
#include "device_control.h"

#define AP_NAME "ESP32-C3-Setup"
#define AP_PASS "12345678"
#define WIFI_TIMEOUT 15000

WebServer server(80);
Preferences prefs;
String savedSSID, savedPASS;

/*/ ---------- OTA WEB PAGE ----------
const char* updatePage =
"<html><body style='text-align:center;'>"
"<h2>ESP32 Web OTA</h2>"
"<form method='POST' action='/update' enctype='multipart/form-data'>"
"<input type='file' name='update'><br><br>"
"<input type='submit' value='Upload'>"
"</form></body></html>";
*/
// ---------- CONNECT WIFI ----------
bool connectWiFi() {
  if (savedSSID == "") return false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSSID.c_str(), savedPASS.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - start > WIFI_TIMEOUT) return false;
  }

  startDeviceServer();

  /*/ OTA handlers only when WiFi connected
  server.on("/update", HTTP_GET, []() {
    server.send(200, "text/html", updatePage);
  });

  server.on("/update", HTTP_POST,
    []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK");
      ESP.restart();
    },
    []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Update.begin(UPDATE_SIZE_UNKNOWN);
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        Update.write(upload.buf, upload.currentSize);
      } else if (upload.status == UPLOAD_FILE_END) {
        Update.end(true);
      }
    }
  );

  server.begin();
  return true;
}
*/
// ---------- WIFI CONFIG PAGE ----------
String wifiPage() {
  String page =
    "<html><body style='text-align:center;'>"
    "<h2>WiFi Setup</h2>"
    "<form action='/save'>"
    "<select name='ssid'>";

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    page += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
  }

  page += "</select><br><br>"
          "<input type='password' name='pass'><br><br>"
          "<input type='submit' value='Save'>"
          "</form></body></html>";
  return page;
}

//void handleConfig() { server.send(200, "text/html", wifiPage()); }

void handleSave() {
  prefs.begin("wifi", false);
  prefs.putString("ssid", server.arg("ssid"));
  prefs.putString("pass", server.arg("pass"));
  prefs.end();

  server.send(200, "text/html", "<h3>Saved! Rebooting...</h3>");
  delay(2000);
  ESP.restart();
}

void handleNotFound() {
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void startConfigPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_NAME, AP_PASS);

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
    startConfigPortal();
  }
}

#endif
