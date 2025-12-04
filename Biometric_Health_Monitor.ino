/***************************************************************
 * THESIS PROJECT: Biometric Health Data Measurement with Live Broadcast
 * Author: Konstantinos Aslanidis
 * * This file contains the complete firmware for the ESP32 to interface
 * with the MAX30100 (HR/SpO2), AD8232 (ECG), MLX90614 (Temperature),
 * and ST7789 TFT display, transmitting data via Blynk.
 *
 * NOTE: This is designed for the Arduino IDE environment running on an ESP32.
 **************************************************************/

// --- BLYNK CONFIGURATION (Replace with your actual credentials) ---
#define BLYNK_TEMPLATE_ID   "TMPL4CgkZQ6NP"
#define BLYNK_TEMPLATE_NAME "Biometrics"
#define BLYNK_AUTH_TOKEN    "MPPi6iOZuhlL7_WgnEtMT2oQJlSCL8Hg"

// --- LIBRARY INCLUDES ---
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>                 // For I2C communication (MAX30100, MLX90614)
#include "MAX30100_PulseOximeter.h" // For HR and SpO2 sensor
#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_MLX90614.h>    // For non-contact temperature sensor
#include <Adafruit_ST7789.h>      // For TFT display

// --- PIN DEFINITIONS ---
#define TFT_CS 5  // Chip Select pin for ST7789
#define TFT_DC 2  // Data/Command pin for ST7789
#define TFT_RST 4 // Reset pin for ST7789
#define BUZZER_PIN 13 // Digital output pin for the alert buzzer
#define ECG_PIN 34 // Analog input pin for AD8232 ECG sensor

// --- WAVEFORM SETTINGS ---
#define WAVEFORM_HEIGHT 160
#define WAVEFORM_START_Y (200 - WAVEFORM_HEIGHT / 2) // Starting Y position for the ECG trace
#define BUFFER_SIZE 240 // Buffer size for storing ECG samples for display

// --- NETWORK AND AUTHENTICATION (Corrected Syntax) ---
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Test_Ard";       // Wi-Fi SSID
char pass[] = "b96hmn6b546xcbcp4nam"; // Wi-Fi Password

// --- OBJECT INITIALIZATION ---
PulseOximeter pox; // MAX30100 Pulse Oximeter object
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // TFT display object
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); // MLX90614 temperature sensor object

// --- DATA BUFFERS AND TIMING ---
int ecgData[BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastSampleTime = 0;
const long sampleInterval = 4; // Sample interval in milliseconds (Approx 250Hz sample rate)
uint32_t tsLastReport = 0;
#define REPORTING_PERIOD_MS 3000 // Interval to read non-ECG sensors and report to Blynk

// --- VITAL SIGNS AND ALERT LOGIC ---
float heartRate = 0, spO2 = 0, objectTemp = 0;
unsigned long lastAlertTime = 0;
const unsigned long alertCooldown = 30000; // 30 seconds cooldown between alerts
int sameReadCount = 0; // Counter for stale data detection
float lastHR = 0, lastSpO2 = 0;

/**
 * @brief Stores a new ECG reading into the circular buffer.
 * @param reading The raw analog value from the ECG sensor.
 */
void updateWaveformBuffer(int reading) {
    // Map raw 0-4095 reading to a displayable range (e.g., 0-1024 for better vertical resolution if needed)
    ecgData[bufferIndex] = reading; 
    bufferIndex = (bufferIndex + 1) % BUFFER_SIZE;
}

/**
 * @brief Renders the buffered ECG waveform on the TFT display.
 */
void displayECGWaveform() {
    // Clear the previous waveform area
    tft.fillRect(0, WAVEFORM_START_Y, 240, WAVEFORM_HEIGHT, ST77XX_BLACK);
    
    // Draw the new waveform trace
    for (int i = 1; i < BUFFER_SIZE; i++) {
        int previousIndex = (bufferIndex + i - 1) % BUFFER_SIZE;
        int currentIndex = (bufferIndex + i) % BUFFER_SIZE;

        // Map the raw ECG data (0-4095) to the display height (WAVEFORM_START_Y to WAVEFORM_START_Y + WAVEFORM_HEIGHT)
        int y1 = map(ecgData[previousIndex], 0, 4095, WAVEFORM_START_Y + WAVEFORM_HEIGHT, WAVEFORM_START_Y);
        int y2 = map(ecgData[currentIndex], 0, 4095, WAVEFORM_START_Y + WAVEFORM_HEIGHT, WAVEFORM_START_Y);
        
        tft.drawLine(i - 1, y1, i, y2, ST77XX_GREEN);
    }
}

void setup() {
    Serial.begin(115200);
    // Initialize Blynk connection
    Blynk.begin(auth, ssid, pass);

    // Initialize I2C communication (SDA=21, SCL=22, Clock Speed=100kHz)
    Wire.begin(21, 22, 100000); 

    // Setup buzzer pin
    pinMode(BUZZER_PIN, OUTPUT);

    // Initialize TFT Display
    tft.init(240, 320);
    tft.setRotation(4); // Landscape orientation (240x320)
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 10);
    tft.println("Initializing...");

    // Initialize Pulse Oximeter (MAX30100)
    if (!pox.begin()) { // Corrected from smallpox.begin()
        Serial.println("Oximeter FAILED!");
        // The original code uses while(1), freezing the ESP. This is kept as per the original logic.
        while (true) {} 
    }

    // Initialize Temperature Sensor (MLX90614)
    if (!mlx.begin()) {
        Serial.println("MLX90614 Sensor NOT Found!");
        // The original code uses while(1), freezing the ESP. This is kept as per the original logic.
        while (true) {}
    }

    Serial.println("Initialization complete.");
}

void loop() {
    yield(); // Required for stable ESP32 operation
    unsigned long currentMillis = millis();

    // 1. MAX30100 update (needs to run constantly for accurate readings)
    pox.update(); // Corrected from smallpox.update()

    // 2. ECG Sampling (Fast, time-sensitive process)
    int ecgValue = analogRead(ECG_PIN);
    if (currentMillis - lastSampleTime >= sampleInterval) {
        lastSampleTime = currentMillis;
        updateWaveformBuffer(ecgValue);

        // Update waveform display every 120 samples (BUFFER_SIZE/2)
        if (bufferIndex % 120 == 0) {
            displayECGWaveform();
        }

        // Send a scaled ECG value to a Blynk V-Pin (V4) for remote monitoring
        int scaledECG = map(ecgValue, 0, 4095, 255, 0);
        Blynk.virtualWrite(V4, scaledECG);
    }

    // 3. VITAL SIGNS REPORTING (Slow, periodic process)
    if (currentMillis - tsLastReport > REPORTING_PERIOD_MS) {
        // Read Temperature
        objectTemp = mlx.readObjectTempC();

        // Read HR and SpO2
        float newHR = pox.getHeartRate(); // Corrected from smallpox.getHeartRate()
        float newSpO2 = pox.getSpO2();     // Corrected from smallpox.getSpO2()

        // Data Validation Logic (As per thesis snippet)
        bool hrValid = newHR > 40 && newHR < 180;
        bool spo2Valid = newSpO2 > 70 && newSpO2 <= 100;
        bool fingerDetected = hrValid && spo2Valid;

        if (fingerDetected) {
            // Stale Data Check
            if (newHR == lastHR && newSpO2 == lastSpO2) {
                sameReadCount++;
                if (sameReadCount >= 2) {
                    Serial.println("Stale MAX30100 data. Resetting...");
                    pox.begin(); // Reinitialize sensor (Corrected)
                    heartRate = 0; // Corrected syntax
                    spO2 = 0;      // Corrected syntax
                }
            } else {
                heartRate = newHR;
                spO2 = newSpO2;
                sameReadCount = 0; // Corrected syntax
            }
        } else {
            heartRate = 0; // No finger detected or invalid data
            spO2 = 0;      // Corrected syntax
        }

        lastHR = newHR;
        lastSpO2 = newSpO2;

        // --- ALERT LOGIC ---
        bool inSafeRange = (heartRate > 40 && heartRate < 120 && spO2 > 90 && objectTemp >= 34 && objectTemp <= 38);

        if (inSafeRange) {
            // Signal safety with a short tone
            tone(BUZZER_PIN, 1000, 100);
        } else {
            // Vitals are out of range
            if (currentMillis - lastAlertTime > alertCooldown) {
                Serial.println("Vitals out of safe range! Sending alert.");
                Blynk.logEvent("out_of_range", "Vitals out of safe range!");
                lastAlertTime = currentMillis;
            }
        }

        // --- BLYNK DATA TRANSMISSION ---
        Blynk.virtualWrite(V1, heartRate);
        Blynk.virtualWrite(V2, spO2);
        Blynk.virtualWrite(V3, objectTemp);

        // --- DISPLAY UPDATE ---
        tft.fillScreen(ST77XX_BLACK);
        
        // HR Display (V1)
        tft.setCursor(10, 10);
        tft.setTextColor((heartRate < 40 || heartRate > 120) ? ST77XX_RED : ST77XX_WHITE);
        tft.print("HR: ");
        tft.print(heartRate);
        tft.println(" bpm");

        // SpO2 Display (V2)
        tft.setCursor(10, 50);
        tft.setTextColor((spO2 < 90) ? ST77XX_RED : ST77XX_WHITE);
        tft.print("SpO2: ");
        tft.print(spO2);
        tft.println(" %");

        // Temperature Display (V3)
        tft.setCursor(10, 90);
        tft.setTextColor((objectTemp < 34 || objectTemp > 38) ? ST77XX_RED : ST77XX_WHITE);
        tft.print("Temp: ");
        tft.print(objectTemp, 1);
        tft.print((char)247); // ASCII character for degree symbol (Corrected syntax)
        tft.println("C");

        // Reset timer for next report (Completed line)
        tsLastReport = currentMillis; 
    }
    
    // 4. Blynk event handling
    Blynk.run();
}
