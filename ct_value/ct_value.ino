// ACS712 Current Sensor Code for ESP32-C3
const int SENSOR_PIN = 1;      // Pin GPIO1 adc
const float ADC_REF = 3.3;     // ADC Reference Voltage
const int ADC_RES = 4095;      // 12-bit Resolution

float sensitivity = 185.0;     
float offsetVoltage = 1.65;    // VCC/2 scaled for 3.3V logic (after divider)

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT);
}

void loop() {
  float avgAdc = 0;
  int samples = 50;

  // Multi-sampling for stability
  for (int i = 0; i < samples; i++) {
    avgAdc += analogRead(SENSOR_PIN);
    delay(1);
  }
  avgAdc /= samples;

  // Convert raw ADC to voltage
  float voltage = (avgAdc * ADC_REF) / ADC_RES;

  float current = ((voltage - offsetVoltage) * 1000) / sensitivity;

  Serial.print("Voltage: ");
  Serial.print(voltage, 3);
  Serial.print("V | Current: ");
  Serial.print(current, 3);
  Serial.println(" A");

  delay(500);
}