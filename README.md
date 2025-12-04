# 📡 Biometric Health Data Measurement with Live Broadcast 
### 🎓 Diploma Thesis – Konstantinos Aslanidis  
International Hellenic University (IHU)

A complete IoMT (Internet of Medical Things) system for **real-time vital-sign monitoring**, **home-care support**, and **live biometric data broadcasting** using an ESP32 microcontroller and the Blynk cloud platform.

---

## 🚀 Project Overview
This repository includes the **firmware**, **wiring documentation**, and **system design** for a real-time biometric monitoring platform.

The system continuously measures:

- ❤️ Heart Rate (HR)  
- 🩸 Blood Oxygen Saturation (SpO₂)  
- 📈 ECG waveform  
- 🌡️ Body Temperature  

Data is collected using biomedical sensors, processed by an **ESP32**, and streamed live to the **Blynk dashboard**, with automated alerts for abnormal vital signs.

---

## ⭐ Key Features

### 📡 Real-Time Monitoring  
Continuous measurement of HR, SpO₂, ECG, and temperature.

### 🔴 Live Data Broadcast  
Real-time visualization through the Blynk web or mobile app.

### 🚨 Alert System  
Alerts are triggered when vital signs exceed safe limits:  
- Local **buzzer alert**  
- Remote **cloud notification**

### 🔄 Stale Data Detection  
Automatic reset of the MAX30100 sensor if readings become unresponsive, ensuring data reliability.

---

## 🧩 Hardware Components

| Component | Function |
|----------|----------|
| **ESP32 Dev Module** | Microcontroller + Wi-Fi |
| **MAX30100** | Pulse oximeter (HR + SpO₂) |
| **AD8232 ECG Module** | ECG waveform measurement |
| **MLX90614** | Non-contact temperature sensor |
| **ST7789 240×320 TFT** | Local display interface |
| **Piezo Buzzer** | Local audio alerts |

---

