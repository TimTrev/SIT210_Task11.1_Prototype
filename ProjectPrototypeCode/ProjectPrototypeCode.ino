#include <LiquidCrystal.h>       // Include the LiquidCrystal library
#include <BH1750FVI.h>           // GY-30 light sensor
#include <DHT.h>                 // DHT11 sensor
#include <Wire.h>                // I2C communication for GY-30 sensor
#include <WiFiNINA.h>            // Wifi communication for Nano 33 IoT
#include "secrets.h"             // Used to include Wi-Fi and IFTTT credentials

// DHT11 settings
#define DHTPIN 2      // Pin for DHT11 data
#define DHTTYPE DHT11 // Define sensor type
DHT dht(DHTPIN, DHTTYPE); // Initialiser for DHT sensor

// GY-30 (BH1750) settings
#define GY30_ADDRESS 0x23 // I2C address
BH1750FVI lightMeter(BH1750FVI::k_DevModeContLowRes); // Creates a BH1750 object for light sensor control

// Soil moisture sensor settings
#define MOISTURE_PIN A0 // Soil moisture sensor pin (analog input)

// LED Pin Definitions
#define RED_LED_PIN 3      // Red LED for Soil Moisture
#define YELLOW_LED_PIN 4   // Yellow LED for Light
#define BLUE_LED_PIN 5     // Blue LED for Temperature
#define GREEN_LED_PIN 6    // Green LED for Humidity

WiFiClient client; // Wifi client object

// Threshold values
#define LIGHT_THRESHOLD 200               // Light less than 200 lux
#define SOIL_THRESHOLD 250               // Soil moisture below 250
#define TEMP_LOW_THRESHOLD 5              // Temperature below 5°C
#define TEMP_HIGH_THRESHOLD 30            // Temperature above 30°C
#define HUMIDITY_THRESHOLD 40             // Humidity above 40%
#define LIGHT_TIME_THRESHOLD 7200000      //setting a timer for 2 hours. Can change to 4 hours by editing value to 14400000

// Variables to track time for light threshold
unsigned long lightStartTime = 0;  // When light first goes below threshold
bool lightThresholdExceeded = false;

unsigned long lastTime = 0;  // Variable to store last update time
unsigned long interval = 60000; // 60 seconds interval for visual text updates

// Initialise LCD with pin configuration
LiquidCrystal lcd(7, 8, A1, A2, A3, A6); // RS, E, D4, D5, D6, D7

// Function to send data to IFTTT
void sendToIFTTT(int temperature, int humidity, float lux, int soilMoisture) {

  // Create the URL for the IFTTT Webhook
  String url = "https://maker.ifttt.com/trigger/" + String(IFTTT_EVENT) + "/with/key/" + String(IFTTT_KEY);
  url += "?value1=" + String(soilMoisture);       // Soil Moisture
  url += "&value2=" + String(lux);                // Light Intensity
  url += "&value3=" + String(temperature);        // Temperature

  // Connect to IFTTT Webhook
  if (client.connect("maker.ifttt.com", 80)) {
    client.print("GET " + url + " HTTP/1.1\r\n");
    client.print("Host: maker.ifttt.com\r\n");
    client.print("Connection: close\r\n\r\n");

    delay(1000);

    // Print response from IFTTT (for debugging)
    while (client.available()) {
      String line = client.readStringUntil('\r');
    }
  } else {
    Serial.println("Failed to connect to IFTTT!");
  }
  client.stop(); // Close the connection
}

void setup() {
  // Start serial communication
  Serial.begin(9600);
  delay(2000);

  // Initialise DHT11 sensor
  dht.begin();

  // Initialise GY-30 light sensor
  lightMeter.begin();

  // Initialise LED pins
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Initialise LCD
  lcd.begin(16, 2); // Initialise a 16x2 LCD
  lcd.setCursor(0, 0);
  lcd.print("Initialising...");
  delay(2000); // Display "Initialising" message
  lcd.clear(); // Clear the display

  // Connect to Wi-Fi
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("WiFi...");
  Serial.print("Connecting to WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Use Wi-Fi credentials from secrets.h

  // Loop until Wi-Fi is connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    lcd.print("."); // Print dots for connecting status on the LCD
    Serial.print(".");
  }

  // Once connected, display the message on LCD
  Serial.println("Connected to WiFi!");
  lcd.clear(); // Clear the display
  lcd.setCursor(0, 0);
  lcd.print("Connected to");
  lcd.setCursor(0, 1);
  lcd.print("WiFi!");
  delay(2000);  // Display WiFi connection message for 2 seconds
  lcd.clear();  // Clear display before displaying sensor readings
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.print("Waiting for data!");
  delay(10000);
  lcd.clear();

}

// Function to handle the light intensity check
void handleLightThreshold(float lux) {
  if (lux < LIGHT_THRESHOLD) {
    if (!lightThresholdExceeded) {
      lightStartTime = millis();
      lightThresholdExceeded = true;
    }
    
    // If light intensity stays below threshold for 2-4 hours (7200000 - 14400000 ms), turn on the LED
    if (millis() - lightStartTime >= LIGHT_TIME_THRESHOLD) {
      digitalWrite(YELLOW_LED_PIN, HIGH); // Turn on LED for low light
      Serial.println("Low light detected. Turning on LED.");
    }
  } else {
    lightThresholdExceeded = false; // Reset light threshold timer
    digitalWrite(YELLOW_LED_PIN, LOW); // Turn off LED when light level is sufficient
  }
}

// Function to handle the temperature threshold check
void handleTemperatureThreshold(int temperature) {
  if (temperature < TEMP_LOW_THRESHOLD) {
    digitalWrite(BLUE_LED_PIN, HIGH);  // Turn on blue LED if temperature is below threshold
    Serial.println("Temperature below threshold! Turning on LED.");
  } else if (temperature > TEMP_HIGH_THRESHOLD) {
    digitalWrite(BLUE_LED_PIN, HIGH);  // Turn on blue LED if temperature exceeds threshold
    Serial.println("Temperature above threshold! Turning on LED.");
  } else {
    digitalWrite(BLUE_LED_PIN, LOW);  // Turn off LED when temperature is within range
  }
}

// Function to handle the humidity threshold check
void handleHumidityThreshold(int humidity) {
  if (humidity > HUMIDITY_THRESHOLD) {
    digitalWrite(YELLOW_LED_PIN, HIGH);  // Turn on yellow LED if humidity exceeds threshold
    Serial.println("Humidity above threshold! Turning on LED.");
  } else {
    digitalWrite(YELLOW_LED_PIN, LOW);   // Turn off LED when humidity is within range
  }
}

// Function to handle the soil moisture threshold check
void handleSoilMoistureThreshold(int soilMoisture) {
  if (soilMoisture < SOIL_THRESHOLD) {
    digitalWrite(RED_LED_PIN, HIGH);  // Turn on red LED if soil moisture is below threshold (needs watering)
    Serial.println("Soil moisture is low. Turning on LED.");
  } else {
    digitalWrite(RED_LED_PIN, LOW);   // Turn off LED if soil moisture is above threshold
  }
}

void loop() {
  // Read temperature and humidity from DHT11
  int humidity = dht.readHumidity();
  int temperature = dht.readTemperature();
  
  // Read light intensity from GY-30 sensor
  float lux = lightMeter.GetLightIntensity();

  // Read soil moisture (analog value from 0 to 1023)
  int soilMoisture = analogRead(MOISTURE_PIN);

  // Check if the readings from DHT11 are valid
  if (isnan(humidity) || isnan(temperature)) { // Checks both humidity and temperature values for NaN (not-a-number)
    Serial.println("Failed to read from DHT sensor!");
  } else {
          // Handle the threshold checks
      handleLightThreshold(lux);
      handleTemperatureThreshold(temperature);
      handleSoilMoistureThreshold(soilMoisture);
      handleHumidityThreshold(humidity);

    // Send data to IFTTT every 60 seconds
    unsigned long currentMillis = millis();
    if (currentMillis - lastTime >= interval) {
      lastTime = currentMillis;

      // Call function to send data to IFTTT
      sendToIFTTT(temperature, humidity, lux, soilMoisture);

      // Print sensor data to the Serial Monitor
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print("°C, Humidity: ");
      Serial.print(humidity);
      Serial.println("%");

      Serial.print("Light Intensity: ");
      Serial.print(lux);
      Serial.println(" lx");

      Serial.print("Soil Moisture: ");
      Serial.println(soilMoisture);

      // Update LCD with sensor readings (without clearing the screen each time)
      lcd.setCursor(0, 0); // Move cursor to the first line
      lcd.print("T: ");
      lcd.print(temperature);
      lcd.print(" H: ");
      lcd.print(humidity);

      lcd.setCursor(0, 1); // Move cursor to the second line
      lcd.print("S: ");
      lcd.print(soilMoisture);
      lcd.print(" L: ");
      lcd.print(lux);
    }
  }
}
