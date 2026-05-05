#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// --- CONFIGURATION ---
const float IMPACT_THRESHOLD = 25.0; 
const int BUZZER_PIN = 27; 
const int RXD2 = 16; 
const int TXD2 = 17;

// --- BUTTON CONFIGURATION ---
const int SOS_BUTTON_PIN = 25;
const int OVERRIDE_BUTTON_PIN = 33; // UPDATED: Now using G33

// State variable to track if we are in an active emergency
bool emergencyActive = false;
unsigned long lastLocationPrintTime = 0; 

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); 

  // Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 

  // Initialize Buttons with Internal Pull-Ups
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OVERRIDE_BUTTON_PIN, INPUT_PULLUP);

  // Initialize GPS
  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  Serial.println("Initializing MPU6050...");
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip. Check your wiring!");
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("System Ready. Waiting for GPS lock...");
  Serial.println("---------------------------------------------------------");
}

void loop() {
  // 1. Constantly feed the GPS buffer
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // 2. Read Sensors & Buttons
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float total_acceleration = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  
  // Note: INPUT_PULLUP means the button reads LOW when pressed
  bool sosPressed = (digitalRead(SOS_BUTTON_PIN) == LOW);
  bool overridePressed = (digitalRead(OVERRIDE_BUTTON_PIN) == LOW);

  // 3. HANDLE OVERRIDE (Resolving the accident)
  if (emergencyActive && overridePressed) {
    emergencyActive = false;           
    digitalWrite(BUZZER_PIN, LOW);     
    Serial.println("\n✅ ACCIDENT RESOLVED. System returning to normal monitoring.");
    Serial.println("---------------------------------------------------------");
    smartDelay(1000); 
  }

  // 4. CHECK FOR TRIGGERS (Only if not already in an emergency)
  if (!emergencyActive) {
    if (total_acceleration > IMPACT_THRESHOLD) {
      emergencyActive = true;
      Serial.println("\n⚠️ G-SPIKE DETECTED! ACCIDENT TRIGGERED! ⚠️");
    } 
    else if (sosPressed) {
      emergencyActive = true;
      Serial.println("\n🚨 MANUAL SOS PUSHED! ACCIDENT TRIGGERED! 🚨");
    }
  }

  // 5. THE EMERGENCY LOOP (Locks the system in alarm mode)
  if (emergencyActive) {
    digitalWrite(BUZZER_PIN, HIGH); 
    
    // Print location every 5 seconds
    if (millis() - lastLocationPrintTime > 5000) {
      if (gps.location.isValid()) {
        Serial.print("EMERGENCY LOCATION -> LAT: ");
        Serial.print(gps.location.lat(), 6);
        Serial.print(" | LNG: ");
        Serial.println(gps.location.lng(), 6);
      } else {
        Serial.println("EMERGENCY ACTIVE: Waiting for valid GPS lock...");
      }
      lastLocationPrintTime = millis();
    }
  }
  
  smartDelay(50); 
}

// --- SMART DELAY ---
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }
  } while (millis() - start < ms);
}