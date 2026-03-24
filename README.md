# 🫀 Health Monitor — ESP32 + Blynk IoT

A real-time health monitoring system built with ESP32 that measures **Heart Rate**, **SpO2 (Blood Oxygen)**, **Body Temperature**, and estimates **Blood Pressure** — all displayed live on the Blynk mobile app.

> Built as a first-semester Computer Engineering project at Pulchowk Engineering College in Lalitpur, Nepal.

---

## 📸 Project Overview

| Measurement | Sensor | Method |
|---|---|---|
| Heart Rate (BPM) | MAX30102 | IR LED reflection |
| SpO2 (%) | MAX30102 | Red + IR light ratio |
| Body Temperature (°C) | MLX90614 | Infrared contactless |
| Blood Pressure (approx) | MAX30102 | Linear estimation from HR |

> ⚠️ Blood pressure is a **rough approximation only** — not medically accurate. It uses a simple linear model based on heart rate correlation.

---

## 🛒 Hardware Required

| Component | Purpose |
|---|---|
| ESP32 DevKit V1 | Main microcontroller |
| MAX30102 | Heart rate + SpO2 sensor |
| MLX90614 | Infrared temperature sensor |
| 2× 1000Ω resistors | I2C pull-up resistors (signal stability) |
| Breadboard | For connections |
| Jumper wires | For wiring |
| USB cable (micro-USB) | Power + programming |

---

## 🔌 Wiring

Both sensors use **I2C** and share the same data pins on the ESP32.

### Sensor → ESP32

| Sensor Pin | ESP32 Pin |
|---|---|
| VIN | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

> Connect **both** MAX30102 and MLX90614 to the same SDA/SCL pins — I2C supports multiple devices on the same bus.

> ⚠️ Always use **3.3V**, never 5V — it will damage the sensors.

### Pull-up Resistors (Important!)

Two **1000Ω pull-up resistors** are used on the I2C lines to strengthen the signal:

```
3.3V ──┬──────────────────┐
       │                  │
      1000Ω               1000Ω
       │                  │
      SDA (GPIO 21)      SCL (GPIO 22)
       │                  │
   [Sensors]          [Sensors]
```

- One resistor connects **3.3V → SDA (GPIO 21)**
- One resistor connects **3.3V → SCL (GPIO 22)**

**Why are these needed?**
I2C is an "open-drain" protocol — sensors communicate by pulling the SDA/SCL lines DOWN to 0V. But when idle, the lines need to sit HIGH at 3.3V. The pull-up resistors act like a spring, holding the lines at 3.3V by default and allowing sensors to pull them low when sending data. Without them, the signal can be weak or noisy — especially on a breadboard with multiple sensors. The resistors ensure clean, reliable communication between the ESP32 and both sensors.

> Standard I2C pull-up values are 4.7kΩ, but 1000Ω works well for short breadboard distances and actually gives a stronger, faster signal.

---

## 💻 Software & Setup

### Requirements
- [VS Code](https://code.visualstudio.com/)
- [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- [Blynk IoT app](https://blynk.io/) (mobile)
- A **2.4GHz WiFi** network (ESP32 does not support 5GHz)

### Libraries (auto-installed by PlatformIO)
```ini
lib_deps =
    sparkfun/SparkFun MAX3010x Pulse and Proximity Sensor Library
    adafruit/Adafruit MLX90614 Library
    adafruit/Adafruit BusIO
    blynkkk/Blynk@^1.3.2
```

---

## ⚙️ Configuration

Before uploading, edit these lines in `src/main.cpp`:

```cpp
// Blynk credentials (from your Blynk device dashboard)
#define BLYNK_TEMPLATE_ID   "your_template_id"
#define BLYNK_TEMPLATE_NAME "Health Monitor"
#define BLYNK_AUTH_TOKEN    "your_auth_token"

// WiFi credentials
char ssid[] = "your_wifi_name";
char pass[] = "your_wifi_password";
```

---

## 📱 Blynk App Setup

1. Create a free account at [blynk.cloud](https://blynk.cloud)
2. Create a new **Template** → Hardware: ESP32, Connection: WiFi
3. Add these **Datastreams** (Virtual Pins):

| Virtual Pin | Name | Type |
|---|---|---|
| V0 | Heart Rate | Integer |
| V1 | SpO2 | Integer |
| V2 | Body Temp | Double |
| V3 | Blood Pressure | String |

4. Create a **Device** from the template and copy your Auth Token into the code
5. In the mobile app, add **Gauge** widgets for V0 and V1, **Label** widgets for V2 and V3

---

## 🚀 How to Upload

1. Clone this repo and open in VS Code with PlatformIO
2. Edit WiFi and Blynk credentials in `src/main.cpp`
3. Connect ESP32 to your laptop via USB
4. Click the **→ Upload** button in the PlatformIO bottom toolbar
5. Open **Serial Monitor** to see live readings
6. Open Blynk app on your phone to see the dashboard

---

## 📊 How It Works

```
ESP32 collects 4 rounds × 100 samples from MAX30102
        ↓
Averages valid readings for stability
        ↓
Reads temperature from MLX90614 (5 reading average)
        ↓
Sends data to Blynk Cloud every 2 seconds
        ↓
Blynk mobile app displays live readings
```

Readings are clamped to realistic ranges to filter out sensor noise:
- Heart Rate: 65–105 BPM
- SpO2: minimum 95%
- Body Temp: 35–39°C

---

## 📁 Project Structure

```
fee-biotechengrg/
├── src/
│   └── main.cpp        ← All your code lives here
├── include/            ← Header files (unused for now)
├── lib/                ← Local libraries (unused for now)
├── test/               ← Tests (unused for now)
└── platformio.ini      ← Board config + library dependencies
```

---

## ⚠️ Known Limitations

- Blood pressure estimation may **not be medically accurate** 
- MAX30102 requires finger to be **held still and firmly** on the sensor for accurate readings
- MLX90614 should be held **1–3 cm away** from wrist or forehead
- ESP32 only works on **2.4GHz WiFi** — does not support 5GHz networks
- Captive portal WiFi (like college networks with login pages) requires additional code

---

## 👨‍💻 Authors

Utsab Raj Bhattarai (Software) || Tangsep Chongbang (Hardware) || Shubham Pokhrel (Report) || Sumyak Limbu (Hardware) || Tushar Aidi (Hardware, Report and Logistics)

Bachelors in Computer Engineering Students at Pulchowk Engineering College

---

## 📄 License

This project is open source and free to use for educational purposes.
