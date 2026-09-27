#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// -------- PROJECT CONFIGURATION --------
const char* ssid = "mpr6324";
const char* password = "qwertyuiop";
const char* serverUrl = "https://emergencyalertapp.onrender.com/data"; 
const char* DEVICE_ID = "ESP32_NODE_01"; // MUST MATCH ANDROID APP CONFIG

#define MOTOR_PIN 25
#define BUTTON_PIN 26
#define PULSE_PIN 34

// -------- GLOBAL OBJECTS --------
MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// -------- SENSOR & AI VARIABLES --------
float totalAcc = 1.0;
float motionEnergy = 0.0;
int bpm = 0;
unsigned long lastBeatInterval = 0;
unsigned long prevBeatInterval = 0;
int lastRawValue = 0;
unsigned long lastPeakTime = 0;
bool fingerPresent = false;

// GPS Persistence
float lastLat = 0.0, lastLon = 0.0;
bool hasValidGPS = false;

// AI Output
String aiDiag = "MONITORING";
int aiRiskScore = 0;

// Thresholds
const int PULSE_THRESHOLD = 2100;
const float FALL_THRESHOLD_LOW = 0.4;
const float FALL_THRESHOLD_HIGH = 2.0;
const unsigned long CANCEL_WINDOW = 10000; 

enum SystemState { IDLE, PRE_ALERT, ALERT_SENT };
SystemState currentState = IDLE;
unsigned long alertStartTime = 0;

void setup() {
    Serial.begin(115200);
    pinMode(MOTOR_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Wire.begin(21, 22);
    mpu.initialize();
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
    connectWiFi();
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nWiFi Connected!");
}

// --- SYMBOLIC AI INFERENCE ENGINE ---
void runAIDiagnostics() {
    aiRiskScore = 0;
    aiDiag = "STABLE";

    if (bpm == 0) { aiDiag = "NO_SIGNAL"; return; }

    // Rule 1: High Heart Rate Base Risk
    if (bpm > 115) aiRiskScore += 50;
    else if (bpm < 50) aiRiskScore += 60;

    // Rule 2: Contextual Motion Check (The "Smart" part)
    if (bpm > 115 && motionEnergy < 0.3) {
        aiRiskScore += 40; // Penalty: High HR while perfectly still
        aiDiag = "ACUTE_RESTING_DISTRESS";
    } else if (bpm > 115 && motionEnergy > 1.2) {
        aiRiskScore -= 30; // Bonus: High HR explained by movement
        aiDiag = "NORMAL_EXERTION";
    }

    // Rule 3: Rhythm Irregularity
    long rhythmDiff = abs((long)lastBeatInterval - (long)prevBeatInterval);
    if (rhythmDiff > 250) {
        aiRiskScore += 30;
        aiDiag = "ARRHYTHMIA_DETECTED";
    }

    // Final Conclusion
    if (aiRiskScore >= 80) aiDiag = "CRITICAL_EVENT";
    else if (aiRiskScore >= 40) aiDiag = "POTENTIAL_DISTRESS";
}

void processSensors() {
    int16_t ax, ay, az;
    mpu.getAcceleration(&ax, &ay, &az);
    float currentAcc = sqrt(pow(ax/16384.0,2) + pow(ay/16384.0,2) + pow(az/16384.0,2));
    totalAcc = currentAcc;

    // Calculate Motion Energy (Running Average of variance from gravity)
    float diff = abs(currentAcc - 1.0);
    motionEnergy = (motionEnergy * 0.9) + (diff * 0.1);

    // Pulse Detection
    int rawPulse = analogRead(PULSE_PIN);
    rawPulse = (rawPulse * 0.2) + (lastRawValue * 0.8);
    lastRawValue = rawPulse;

    if (rawPulse > 1500) {
        fingerPresent = true;
        if (rawPulse > PULSE_THRESHOLD && (millis() - lastPeakTime > 400)) {
            unsigned long now = millis();
            prevBeatInterval = lastBeatInterval;
            lastBeatInterval = now - lastPeakTime;
            if (lastBeatInterval < 2000) bpm = 60000 / lastBeatInterval;
            lastPeakTime = now;
            runAIDiagnostics(); 
        }
    } else {
        fingerPresent = false;
        if (millis() - lastPeakTime > 3000) { bpm = 0; aiDiag = "IDLE"; }
    }
}

void sendEmergency() {
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    WiFiClientSecure client;
    client.setInsecure(); 

    float sendLat = (gps.location.isValid() && gps.location.lat() != 0.0) ? gps.location.lat() : lastLat;
    float sendLon = (gps.location.isValid() && gps.location.lng() != 0.0) ? gps.location.lng() : lastLon;

    HTTPClient http;
    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    char jsonBuffer[256];
    snprintf(jsonBuffer, sizeof(jsonBuffer), 
             "{\"device_id\":\"%s\", \"lat\":%.6f, \"lon\":%.6f, \"bpm\":%d, \"diag\":\"%s\", \"alert_id\":%lu}", 
             DEVICE_ID, sendLat, sendLon, bpm, aiDiag.c_str(), millis());

    int httpCode = http.POST(jsonBuffer);
    Serial.printf("Cloud Sync: %d\n", httpCode);
    http.end();
}

void loop() {
    while (gpsSerial.available()) gps.encode(gpsSerial.read());
    if (gps.location.isValid() && gps.location.lat() != 0.0) {
        lastLat = gps.location.lat(); lastLon = gps.location.lng(); hasValidGPS = true;
    }

    processSensors();

    bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);
    bool fallDetected = (totalAcc < FALL_THRESHOLD_LOW || totalAcc > FALL_THRESHOLD_HIGH);
    bool aiCritical = (aiRiskScore >= 80);

    switch (currentState) {
        case IDLE:
            if (fallDetected || aiCritical) {
                Serial.printf("🚨 TRIGGER: %s\n", fallDetected ? "FALL" : "AI");
                currentState = PRE_ALERT;
                alertStartTime = millis();
                digitalWrite(MOTOR_PIN, HIGH); 
            }
            break;
        case PRE_ALERT:
            if (buttonPressed) { 
                digitalWrite(MOTOR_PIN, LOW); 
                currentState = IDLE; 
                delay(1000); 
            } 
            else if (millis() - alertStartTime > CANCEL_WINDOW) {
                digitalWrite(MOTOR_PIN, LOW);
                sendEmergency();
                currentState = ALERT_SENT;
                alertStartTime = millis();
            }
            break;
        case ALERT_SENT:
            if (millis() - alertStartTime > 5000 || buttonPressed) currentState = IDLE;
            break;
    }
}