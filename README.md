# Context-Aware IoT Wearable: A Symbolic AI Approach to Emergency Detection

### 🏥 Final Year Engineering Project
An end-to-end medical IoT ecosystem designed to detect physical falls and cardiac distress using **Edge-AI Sensor Fusion**. This system distinguishes between physiological exertion (e.g., running) and actual medical emergencies to minimize false positives.

---

## 🚀 System Architecture
The project follows a 4-layer IoT architecture:
1. **Perception Layer (Hardware):** ESP32, MPU6050 (Inertia), Pulse Sensor (Vitals), and NEO-6M (GPS).
2. **Cognition Layer (Edge AI):** A Symbolic AI Expert System running locally on the ESP32 performing multivariate inference.
3. **Transport Layer (Cloud):** A RESTful Flask API hosted on **Render.com** via HTTPS.
4. **Presentation Layer (Dashboard):** A native Android application with a persistent Foreground Service and Google Maps integration.

---

## ✨ Key Features
- **AI-Driven Sensor Fusion:** Fuses BPM and Motion data to ignore high heart rates caused by exercise while flagging resting tachycardia.
- **24/7 Monitoring:** Android Foreground Service with CPU WakeLocks ensures the app detects alerts even when closed.
- **Intelligent Haptics:** A 10-second vibration "Pre-Alert" window allows users to cancel false detections manually.
- **Global Traceability:** Cloud-synced incident history with precise GPS mapping for every past alert.
- **Offline Resilience:** Automatically caches and sends the "Last Known Good" GPS location if the indoor signal is lost.

---

## 🛠️ Hardware Components
- **ESP32 DEV Board:** The system brain handling sensor data and secure WiFi transmission.
- **NEO-6M GPS:** Provides real-time Latitude and Longitude coordinates.
- **MPU6050 IMU:** 6-axis motion tracking for fall detection (Weightlessness -> Impact -> Stillness).
- **Pulse Sensor:** Measures heart rate and rhythm stability.
- **Vibration Motor:** Provides tactile feedback for the Pre-Alert phase.
- **Push Button:** User input for manual alert cancellation.

---

## 💻 Tech Stack
- **Firmware:** Embedded C++ (Arduino IDE)
- **Mobile:** Java, Android SDK (Foreground Services, Broadcast Receivers)
- **Backend:** Python, Flask, Gunicorn
- **Cloud:** Render.com (PaaS)
- **AI Model:** Symbolic AI / Rule-Based Inference Engine

---

## 🚦 Getting Started

### Hardware Setup
1. Connect MPU6050 to I2C pins (21, 22).
2. Connect NEO-6M GPS to UART pins (16, 17).
3. Connect Pulse Sensor to Analog pin (34).
4. Connect Vibration Motor and Button to GPIO (25, 26).

### Cloud Setup
1. Deploy the `/flask-backend` to Render.com.
2. Note your unique `onrender.com` URL.

### Android Setup
1. Install the provided APK.
2. Navigate to **CONFIG**.
3. Enter your Cloud URL and Port **443**.
4. Set battery usage to **"Unrestricted"** for 24/7 reliability.

---

## 🧠 The AI Logic (Expert System)
The system calculates a **Dynamic Risk Index (DRI)** based on medical knowledge thresholds:
- **Rule 1:** If `BPM > 115` AND `Motion Energy < 0.2g` -> **CRITICAL DISTRESS** (+90 Risk).
- **Rule 2:** If `BPM > 115` AND `Motion Energy > 1.5g` -> **NORMAL EXERTION** (Deduct Risk).
- **Rule 3:** If `Impact > 3.0g` AND `Stillness > 5s` -> **UNCONSCIOUS FALL** (100% Risk).

---

## 📝 License
This project is for educational purposes as part of a Final Year Project curriculum.
