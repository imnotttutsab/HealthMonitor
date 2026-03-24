// ===== BLYNK CONFIG - must be at very top =====
#define BLYNK_TEMPLATE_ID   "TMPL6jdqX86M6"
#define BLYNK_TEMPLATE_NAME "Health Monitor"
#define BLYNK_AUTH_TOKEN    "p7k02cMnkdIW6MeMjaNyC3ymqSpkEWV-"
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <Adafruit_MLX90614.h>

// ===== WIFI CREDENTIALS =====
char ssid[] = "shivpradhan_2";       // your WiFi name
char pass[] = "CLB44917B9";    // your WiFi password

// ===== BLYNK TIMER =====
BlynkTimer timer;

// ===== SENSOR OBJECTS =====
MAX30105 heartSensor;
Adafruit_MLX90614 tempSensor = Adafruit_MLX90614();

// ===== RAW DATA BUFFERS =====
uint32_t irBuffer[100];
uint32_t redBuffer[100];

// ===== RESULT VARIABLES =====
// These are global so sendToBlynk() can access them
int32_t heartRate;
int8_t  validHeartRate;
int32_t spo2;
int8_t  validSPO2;
float   bodyTemp;

// ===== SENDS DATA TO BLYNK APP EVERY 2 SECONDS =====
void sendToBlynk() {
  if (validHeartRate == 1 && heartRate > 20 && heartRate < 250) {

    // Clamp heart rate between 65 and 105
    int32_t clampedHR = heartRate;
    if (clampedHR < 65)  clampedHR = 65;
    if (clampedHR > 105) clampedHR = 105;

    Blynk.virtualWrite(V0, clampedHR);

    // Blood pressure based on clamped HR
    float systolic  = 0.5 * clampedHR + 70.0;
    float diastolic = 0.3 * clampedHR + 50.0;
    String bp = String(systolic, 0) + "/" + String(diastolic, 0) + " mmHg";
    Blynk.virtualWrite(V3, bp);
  }

  if (validSPO2 == 1 && spo2 > 50) {

    // Clamp SpO2 — minimum 95
    int32_t clampedSpO2 = spo2;
    if (clampedSpO2 < 95) clampedSpO2 = 95;

    Blynk.virtualWrite(V1, clampedSpO2);
  }

  // Clamp body temp between 35 and 39
  float clampedTemp = bodyTemp;
  if (clampedTemp < 35.0) clampedTemp = 35.0;
  if (clampedTemp > 39.0) clampedTemp = 39.0;

  Blynk.virtualWrite(V2, clampedTemp);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println("Booting...");

  // ===== CONNECT TO WIFI + BLYNK =====
  Serial.println("Connecting to WiFi and Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Connected!");

  // ===== INIT MAX30102 =====
  if (!heartSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("ERROR: MAX30102 not found! Check wiring.");
    while (true) { delay(1000); }
  }
  Serial.println("MAX30102 OK!");
  heartSensor.setup();
  heartSensor.setPulseAmplitudeRed(0x0A);
  heartSensor.setPulseAmplitudeGreen(0);

  // ===== INIT MLX90614 =====
  if (!tempSensor.begin()) {
    Serial.println("ERROR: MLX90614 not found! Check wiring.");
    while (true) { delay(1000); }
  }
  Serial.println("MLX90614 OK!");

  // ===== BLYNK TIMER: send data every 10 seconds =====
  timer.setInterval(10000L, sendToBlynk);

  Serial.println("==================================");
  Serial.println("Setup done! Place finger on sensor");
  Serial.println("==================================");
}

void loop() {
  Blynk.run();
  timer.run();

  // ===== COLLECT MULTIPLE ROUNDS AND AVERAGE =====
  // We take 4 rounds of 100 samples each = ~400 samples total
  // Then average them for a much more stable reading

  const int ROUNDS = 4;
  int32_t   hrSum      = 0;
  int32_t   spo2Sum    = 0;
  int       hrCount    = 0;  // only count valid readings
  int       spo2Count  = 0;

  for (int round = 0; round < ROUNDS; round++) {
    Serial.print("Collecting round ");
    Serial.print(round + 1);
    Serial.print(" of ");
    Serial.println(ROUNDS);

    // Collect 100 samples per round
    for (byte i = 0; i < 100; i++) {
      while (!heartSensor.available()) {
        heartSensor.check();
      }
      redBuffer[i] = heartSensor.getRed();
      irBuffer[i]  = heartSensor.getIR();
      heartSensor.nextSample();
    }

    // Calculate HR and SpO2 for this round
    int32_t roundHR, roundSpo2;
    int8_t  roundValidHR, roundValidSpo2;

    maxim_heart_rate_and_oxygen_saturation(
      irBuffer, 100, redBuffer,
      &roundSpo2,  &roundValidSpo2,
      &roundHR,    &roundValidHR
    );

    // Only add to sum if the reading is valid and realistic
    if (roundValidHR == 1 && roundHR > 20 && roundHR < 250) {
      hrSum += roundHR;
      hrCount++;
    }
    if (roundValidSpo2 == 1 && roundSpo2 > 50) {
      spo2Sum += roundSpo2;
      spo2Count++;
    }
  }

  // ===== COMPUTE AVERAGES =====
  if (hrCount > 0) {
    heartRate      = hrSum / hrCount;
    validHeartRate = 1;
  } else {
    validHeartRate = 0;
  }

  if (spo2Count > 0) {
    spo2      = spo2Sum / spo2Count;
    validSPO2 = 1;
  } else {
    validSPO2 = 0;
  }

  // ===== READ TEMPERATURE (average 5 readings) =====
  float tempSum = 0;
  for (int i = 0; i < 5; i++) {
    tempSum += tempSensor.readObjectTempC();
    delay(200);
  }
  bodyTemp       = tempSum / 5;
  float roomTemp = tempSensor.readAmbientTempC();

  // ===== PRINT RESULTS =====
  Serial.println("========== FINAL AVERAGED READINGS ==========");
  if (validHeartRate == 1) {
    Serial.print("Heart Rate:  "); Serial.print(heartRate); Serial.println(" BPM");
    float sys = 0.5 * heartRate + 70.0;
    float dia = 0.3 * heartRate + 50.0;
    Serial.print("BP Estimate: "); Serial.print(sys, 0);
    Serial.print("/");            Serial.print(dia, 0); Serial.println(" mmHg");
  } else {
    Serial.println("Heart Rate:  No finger / bad reading");
  }

  if (validSPO2 == 1) {
    Serial.print("SpO2:        "); Serial.print(spo2); Serial.println(" %");
  } else {
    Serial.println("SpO2:        No finger / bad reading");
  }

  Serial.print("Body Temp:   "); Serial.print(bodyTemp); Serial.println(" C");
  Serial.print("Room Temp:   "); Serial.print(roomTemp); Serial.println(" C");
  Serial.println("(Averaged over 4 rounds)");
  Serial.println("=============================================");
}

