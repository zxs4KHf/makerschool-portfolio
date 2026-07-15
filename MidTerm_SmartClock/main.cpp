#include <WiFi.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "time.h"

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// NTP Server
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 8 * 3600; // Beijing time (UTC+8)
const int   daylightOffset_sec = 0;

// OLED Setup (I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// DS18B20 Setup
const int oneWireBus = 4; // GPIO 4
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  
  // Init OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, "Connecting WiFi...");
  u8g2.sendBuffer();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");

  // Init NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Init Temp Sensor
  sensors.begin();
}

void loop() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);

  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S", &timeinfo);
  
  char dateStringBuff[50];
  strftime(dateStringBuff, sizeof(dateStringBuff), "%Y-%m-%d", &timeinfo);

  char tempStringBuff[20];
  snprintf(tempStringBuff, sizeof(tempStringBuff), "Temp: %.1f C", temperatureC);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(20, 25, timeStringBuff);
  
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(30, 45, dateStringBuff);
  u8g2.drawStr(30, 60, tempStringBuff);
  
  u8g2.sendBuffer();

  delay(1000);
}
