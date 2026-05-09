#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <WiFi.h>              // NEW: Wi-Fi Library
#include <FirebaseESP32.h>     // NEW: Firebase Library

Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

// --- WI-FI & FIREBASE CONFIGURATION (CHANGE THESE!) ---
#define WIFI_SSID "mat02"        // e.g., "Ahmad iPhone"
#define WIFI_PASSWORD "1sampai9" // e.g., "password123"
#define FIREBASE_HOST "fyp-helmet-c9079-default-rtdb.asia-southeast1.firebasedatabase.app"  // Paste the URL (Remove "https://" and the trailing "/")
#define FIREBASE_AUTH // Paste the Web API Key

// Firebase Data Objects
FirebaseData firebaseData;
FirebaseAuth auth;
FirebaseConfig config;

// --- HARDWARE CONFIGURATION ---
const float IMPACT_THRESHOLD = 25.0; 
const int BUZZER_PIN = 27; 
const int RXD2 = 16; 
const int TXD2 = 17;
const int SOS_BUTTON_PIN = 25;
const int OVERRIDE_BUTTON_PIN = 33; 

// State variables
bool emergencyActive = false;
unsigned long lastLocationPrintTime = 0; 

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); 

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(OVERRIDE_BUTTON_PIN, INPUT_PULLUP);

  gpsSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // --- CONNECT TO WI-FI ---
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\n✅ Wi-Fi Connected!");

  // --- CONNECT TO FIREBASE ---
  config.host = FIREBASE_HOST;
  config.api_key = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("✅ Firebase Initialized!");
  
  // Set initial "Safe" state in the database
  Firebase.setString(firebaseData, "/Helmet_1/Status", "SAFE");
  Firebase.setFloat(firebaseData, "/Helmet_1/Latitude", 0.0);
  Firebase.setFloat(firebaseData, "/Helmet_1/Longitude", 0.0);

  // --- INITIALIZE SENSORS ---
  Serial.println("Initializing MPU6050...");
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip!");
    while (1) { delay(10); }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("System Ready. Waiting for GPS lock...");
  Serial.println("---------------------------------------------------------");
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float total_acceleration = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));
  
  bool sosPressed = (digitalRead(SOS_BUTTON_PIN) == LOW);
  bool overridePressed = (digitalRead(OVERRIDE_BUTTON_PIN) == LOW);

  // --- HANDLE OVERRIDE ---
  if (emergencyActive && overridePressed) {
    emergencyActive = false;           
    digitalWrite(BUZZER_PIN, LOW);     
    Serial.println("\n✅ ACCIDENT RESOLVED.");
    
    // Update Firebase to show the worker is safe again
    Firebase.setString(firebaseData, "/Helmet_1/Status", "SAFE");
    
    smartDelay(1000); 
  }

  // --- CHECK FOR TRIGGERS ---
  if (!emergencyActive) {
    if (total_acceleration > IMPACT_THRESHOLD) {
      emergencyActive = true;
      Serial.println("\n⚠️ G-SPIKE DETECTED!");
      Firebase.setString(firebaseData, "/Helmet_1/Status", "EMERGENCY - IMPACT DETECTED");
    } 
    else if (sosPressed) {
      emergencyActive = true;
      Serial.println("\n🚨 MANUAL SOS PUSHED!");
      Firebase.setString(firebaseData, "/Helmet_1/Status", "EMERGENCY - SOS BUTTON PUSHED");
    }
  }

  // --- THE EMERGENCY LOOP ---
  if (emergencyActive) {
    digitalWrite(BUZZER_PIN, HIGH); 
    
    // Update Firebase with location every 5 seconds
    if (millis() - lastLocationPrintTime > 5000) {
      if (gps.location.isValid()) {
        float currentLat = gps.location.lat();
        float currentLng = gps.location.lng();
        
        Serial.print("Sending to Firebase -> LAT: ");
        Serial.print(currentLat, 6);
        Serial.print(" | LNG: ");
        Serial.println(currentLng, 6);

        // Push live coordinates to the cloud
        Firebase.setFloat(firebaseData, "/Helmet_1/Latitude", currentLat);
        Firebase.setFloat(firebaseData, "/Helmet_1/Longitude", currentLng);
        
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
