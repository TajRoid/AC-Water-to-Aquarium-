# 💧 AC Water to Aquarium

Reclaim pure AC condensate water into safe, mineral-balanced aquarium water for fish and aquatic plants using an automated ESP32 monitoring and dosing system.

![Status](https://img.shields.io/badge/status-completed-brightgreen)
![Date](https://img.shields.io/badge/date-October%202026-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Category](https://img.shields.io/badge/category-Embedded%20Systems%20%2F%20Aquaculture-green)

---

## 🛠️ Hardware & Components List

| Component / Tool | Function | Pin / Specification |
|---|---|---|
| **ESP32 Microcontroller** | System controller & UI engine | Standard ESP32 Dev Board |
| **pH Sensor + Module** | Water acidity measurement | **GPIO 35** (Analog) |
| **TDS Sensor Module** | Total Dissolved Solids measurement | **GPIO 34** (Analog) |
| **OLED Display SH1106 1.3"** | Visual UI menu & live parameters | **GPIO 21 (SDA) / GPIO 22 (SCL)** |
| **Servo 360° DS04-NFC #1** | Mineral powder dispenser | **GPIO 25** (PWM) |
| **Servo 360° DS04-NFC #2** | pH Buffer dispenser | **GPIO 26** (PWM) |
| **Push Button #1 (PILIH)** | Menu navigation | **GPIO 27** (`INPUT_PULLUP` to GND) |
| **Push Button #2 (OK)** | Confirm / Manual dosing trigger | **GPIO 14** (`INPUT_PULLUP` to GND) |
| **Air Pump & Stone** | 24h aeration (dissolved oxygen) | Standard 5V/220V Aerator |
| **Filtering Media** | Physical & chemical purification | Filter cotton + Activated Carbon |

---

## 🧪 Key Parameters & Water Targets

| Parameter | Raw AC Water | Target Range (Fish) | Target Range (Plant) |
|---|---|---|---|
| **TDS** | `< 20 ppm` | **190 – 225 ppm** | **130 – 170 ppm** |
| **pH** | `5.5 – 6.5` | **6.5 – 7.5** | **5.5 – 6.5** |
| **DO / Aeration** | Very Low | `> 5.0 mg/L` (Aerated 24h) | `> 5.0 mg/L` |

---

## 🔄 Quick Process Flow
