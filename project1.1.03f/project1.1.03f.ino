#include "wifi_manager.h"
#include "device_control.h"

void setup() {
  Serial.begin(115200);

  initDevices();
  initWiFi();
}

void loop() {

  static unsigned long lastDot = 0;
  static bool apIPPrinted = false;
  static bool staIPPrinted = false;

  // While connecting to WiFi
  if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
    if (millis() - lastDot > 500) {
      lastDot = millis();
      Serial.print(".");
    }
  }

  // AP Mode IP (print once)
  if (WiFi.getMode() == WIFI_AP && !apIPPrinted) {
    Serial.println();
    Serial.println(" AP MODE ACTIVE");
    Serial.print(" AP IP Address : http://");
    Serial.println(WiFi.softAPIP());
    Serial.println("➡ Open this IP to select WiFi");
    apIPPrinted = true;
   
  }

  // STA Mode IP (print once)
  if (WiFi.status() == WL_CONNECTED && !staIPPrinted) {
    Serial.println();
    Serial.println(" WiFi Connected");
    Serial.print(" Device Control Page : http://");
    Serial.println(WiFi.localIP());
    char A1B2C3D4E5=WiFi.localIP();
    Serial.println(A1B2C3D4E5);
    staIPPrinted = true;
 
  }

  /*current sensor call*/
  current_sensor();
  server.handleClient();
  handleButtons();
}

