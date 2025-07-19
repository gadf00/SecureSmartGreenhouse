# 🌱 SecureSmartGreenhouse

A secure and intelligent greenhouse system built with ESP32, Node-RED, MQTT, and Mutual TLS.

> Developed for the *IoT Security* course by Gaetano De Filippo  
> Project goal: combine **automation** with **strong communication security**.

---

## 🔐 Project Summary

**SecureSmartGreenhouse** is a smart irrigation system designed to:
- Monitor **soil moisture**, **air temperature**, and **humidity**.
- Automatically or manually **control irrigation pumps**.
- Securely communicate using **MQTT over Mutual TLS (mTLS)**.
- Display and control the system via a **Node-RED dashboard**.

---

## ⚙️ Architecture

- **Sensor Node (ESP32)**  
  - 1x DHT22 sensor (Temperature & Humidity)  
  - 2x Soil Moisture sensors  
  - Publishes data to `esp32/data` topic

- **Actuator Node (ESP32)**  
  - Controls 2 pumps via 2-channel relay  
  - Receives commands via `esp32/command`  
  - Sends confirmations on `esp32/confirm`

- **Server (Node-RED + Mosquitto Broker)**  
  - Handles logic for auto/manual irrigation  
  - Displays real-time gauges & historical graphs  
  - Sends notifications for:
    - Completed irrigations
    - Out-of-range temperature or humidity

---

## 🔐 Mutual TLS (mTLS)

The system uses **Mutual TLS** for secure communication.  
Each device uses its own **certificate and key** signed by a **local Certificate Authority (CA)**.

| Component     | Certificate Type     |
|---------------|----------------------|
| ESP32 Sensor  | Client certificate   |
| ESP32 Actuator| Client certificate   |
| Node-RED      | Client certificate   |
| Mosquitto     | Server certificate (signed by CA) |
