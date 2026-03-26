#include "wifi_manager.h"
#include "device_control.h"

unsigned long lastIPPrint = 0;

void setup() {

  Serial.begin(115200);
  delay(3000);

  Serial.println("\n--- ESP32 STARTED ---");

  initDevices();
  initWiFi();
}

void loop() {

  server.handleClient();

  checkButtons();

  if (millis() - lastIPPrint >= 5000) {

    lastIPPrint = millis();

    if (WiFi.getMode() == WIFI_AP) {
      Serial.print("AP Mode IP: ");
      Serial.println(WiFi.softAPIP());
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Device Control Page: http://");
      Serial.println(WiFi.localIP());
      Serial.println("OTA Page: /update");
    }
  }
}