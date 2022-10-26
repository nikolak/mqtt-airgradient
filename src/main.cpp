/*

This code has been modified from https://github.com/airgradienthq/arduino/blob/d5c8af68a0d2b327c147e3f81c0da9141e1d1d95/examples/DIY_PRO/DIY_PRO.ino
I've modified the original code a bit, and added the mqtt functionality to it as I wanted it to report to mqtt instead of airgradient's servers.

It's not the most polished code, but it works for me.

-- Original code header --
This is the code for the AirGradient DIY PRO Air Quality Sensor with an ESP8266 Microcontroller.

It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

Build Instructions: https://www.airgradient.com/open-airgradient/instructions/diy-pro/

Kits (including a pre-soldered version) are available: https://www.airgradient.com/open-airgradient/kits/

The codes needs the following libraries installed:
“WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
“U8g2” by oliver tested with version 2.32.15
“SGP30” by Rob Tilaart tested with Version 0.1.5

Configuration:
Please set in the code below the configuration parameters.

If you have any questions please visit our forum at https://forum.airgradient.com/

If you are a school or university contact us for a free trial on the AirGradient platform.
https://www.airgradient.com/

MIT License

*/
#include <Arduino.h>

#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#include "SGP30.h"
#include <U8g2lib.h>
#include <config.h>

AirGradient ag = AirGradient();
SGP30 SGP;

// Display bottom right
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, /* reset=*/U8X8_PIN_NONE);

// Replace above if you have display on top left
// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R3, /* reset=*/ U8X8_PIN_NONE);

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

void callback(char *topic, byte *payload, unsigned int length)
{
  // handle message arrived
}

WiFiClient client;
PubSubClient mqttclient(MQTT_HOST, MQTT_PORT, callback, client);

// CONFIGURATION END

unsigned long currentMillis = 0;

const int oledInterval = 5000;
unsigned long previousOled = 0;

const int sendToServerInterval = 10000;
unsigned long previoussendToServer = 0;

const int tvocInterval = 10000;
unsigned long previousTVOCupdate = 0;
int TVOC = 0;
int previous_TVOC_value = -1;

const int co2Interval = 10000;
unsigned long previousCo2Update = 0;
int Co2 = 0;
int previous_Co2_value = -1;

const int pm25Interval = 10000;
unsigned long previousPm25update = 0;
int pm25 = 0;
int previous_pm25_value = -1;

const int tempHumInterval = 10000;
unsigned long previousTempHumUpdate = 0;
float temp = 0;
float previous_temp_value = -1;
int hum = 0;
int previous_hum_value = -1;

// pre declare empty functions that will be used later
void updateTVOC();
void updateOLED();
void updateCo2();
void updatePm25();
void updateTempHum();
void sendToServer();
void updateOLED2(String ln1, String ln2, String ln3);
void connectToWifi();

void setup()
{
  Serial.begin(115200);
  
  u8g2.begin();
  updateOLED();

  connectToWifi();

  Serial.println(SGP.begin());
  SGP.GenericReset();

  ag.CO2_Init();
  ag.PMS_Init();
  ag.TMP_RH_Init(0x44);
}

void loop()
{
  currentMillis = millis();
  updateTVOC();
  updateOLED();
  updateCo2();
  updatePm25();
  updateTempHum();
  sendToServer();
}

void updateTVOC()
{
  if (currentMillis - previousTVOCupdate >= tvocInterval)
  {
    previousTVOCupdate += tvocInterval;
    previous_TVOC_value = TVOC;
    SGP.measure(true);
    TVOC = SGP.getTVOC();
    Serial.print("TVOC: ");
    Serial.print(previous_TVOC_value);
    Serial.print(" -> ");
    Serial.println(TVOC);
  }
}

void updateCo2()
{
  if (currentMillis - previousCo2Update >= co2Interval)
  {
    previousCo2Update += co2Interval;
    // create variable previous_Co2_value that is equal to the current value of Co2
    previous_Co2_value = Co2;
    Co2 = ag.getCO2_Raw();
    Serial.print("CO2: ");
    Serial.print(previous_Co2_value);
    Serial.print(" -> ");
    Serial.println(Co2);
  }
}

void updatePm25()
{
  if (currentMillis - previousPm25update >= pm25Interval)
  {
    previousPm25update += pm25Interval;
    previous_pm25_value = pm25;
    pm25 = ag.getPM2_Raw();
    Serial.print("PM2.5: ");
    Serial.print(previous_pm25_value);
    Serial.print(" -> ");
    Serial.println(pm25);
  }
}

void updateTempHum()
{
  if (currentMillis - previousTempHumUpdate >= tempHumInterval)
  {
    previousTempHumUpdate += tempHumInterval;
    previous_temp_value = temp;
    previous_hum_value = hum;
    TMP_RH result = ag.periodicFetchData();
    temp = result.t;
    hum = result.rh;
    Serial.print("Temp: ");
    Serial.print(previous_temp_value);
    Serial.print(" -> ");
    Serial.println(temp);
    Serial.print("Hum: ");
    Serial.print(previous_hum_value);
    Serial.print(" -> ");
    Serial.println(hum);
  }
}

void configTime()
{
  configTime(0, 0, NTP_SERVER, NTP_SERVER2);
  Serial.println("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

// connect to ntp server and get time and return if the time is after 10pm
bool isNight()
{
  // configTime for the europe/berlin timezone
  configTime(3600, 3600, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  return timeinfo->tm_hour >= 22 || timeinfo->tm_hour < 6;
}

// Wifi Manager
void connectToWifi()
{
  updateOLED2("Connecting to", "Wifi SSID", SSID);
  WiFi.begin(SSID, SSID_PASSWORD);
  delay(5000);
  if (WiFi.status() != WL_CONNECTED)
  {
    updateOLED2("booting into", "offline mode", "");
    Serial.println("failed to connect and hit timeout");
    delay(6000);
  }
}

// Calculate PM2.5 US AQI
int PM_TO_AQI_US(int pm02)
{
  if (pm02 <= 12.0)
    return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4)
    return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4)
    return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4)
    return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4)
    return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4)
    return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4)
    return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else
    return 500;
};

void updateOLED()
{
  // turn off the oled if it is night
  if (isNight())
  {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(0, 10, "It's night");
    u8g2.sendBuffer();
    delay(5000);
    u8g2.setPowerSave(1);
    return;
  }
  // turn on the oled if it is day
  u8g2.setPowerSave(0);
  if (currentMillis - previousOled >= oledInterval)
  {
    previousOled += oledInterval;

    String ln3;
    String ln1 = "PM:" + String(pm25) + " CO2:" + String(Co2);
    String ln2 = "AQI:" + String(PM_TO_AQI_US(pm25)) + " TVOC:" + String(TVOC);

    if (inF)
    {
      ln3 = "F:" + String((temp * 9 / 5) + 32) + " H:" + String(hum) + "%";
    }
    else
    {
      ln3 = "C:" + String(temp) + " H:" + String(hum) + "%";
    }
    updateOLED2(ln1, ln2, ln3);
  }
}

void updateOLED2(String ln1, String ln2, String ln3)
{
  u8g2.firstPage();
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 30, String(ln2).c_str());
    u8g2.drawStr(1, 50, String(ln3).c_str());
  } while (u8g2.nextPage());
}

void sendToServer()
{
  if (currentMillis - previoussendToServer >= sendToServerInterval)
  {
    previoussendToServer += sendToServerInterval;

    if (WiFi.status() == WL_CONNECTED)
    {
      if (mqttclient.connect(MQTT_HOST, MQTT_USERNAME, MQTT_PASSWORD))
      {
        mqttclient.publish((String(MQTT_TOPIC) + "esp_chip_id").c_str(), String(ESP.getChipId()).c_str());
        if ((previous_pm25_value != pm25 && pm25 != 0) || (pm25 == 0 && previous_pm25_value == 0))
        {
          mqttclient.publish((String(MQTT_TOPIC) + "pm25").c_str(), String(pm25).c_str());
          mqttclient.publish((String(MQTT_TOPIC) + "aqi").c_str(), String(PM_TO_AQI_US(pm25)).c_str());
          if (PM_TO_AQI_US(pm25) <= 50)
          {
            mqttclient.publish((String(MQTT_TOPIC) + "aqi_level").c_str(), "Good");
          }
          else if (PM_TO_AQI_US(pm25) <= 100)
          {
            mqttclient.publish((String(MQTT_TOPIC) + "aqi_level").c_str(), "Moderate");
          }
          else if (PM_TO_AQI_US(pm25) <= 150)
          {
            mqttclient.publish((String(MQTT_TOPIC) + "aqi_level").c_str(), "Unhealthy for Sensitive Groups");
          }
          else if (PM_TO_AQI_US(pm25) <= 200)
          {
            mqttclient.publish((String(MQTT_TOPIC) + "aqi_level").c_str(), "Unhealthy");
          }
          else if (PM_TO_AQI_US(pm25) <= 300)
          {
            mqttclient.publish((String(MQTT_TOPIC) + "aqi_level").c_str(), "Very Unhealthy");
          }
          else if (PM_TO_AQI_US(pm25) <= 500)
          {
            mqttclient.publish((String(MQTT_TOPIC) + "aqi_level").c_str(), "Hazardous");
          }
          else
          {
            mqttclient.publish((String(MQTT_TOPIC) + "aqi_level").c_str(), "Unknown");
          }
        }
        if ((previous_Co2_value != Co2 && Co2 != 0) || (Co2 == 0 && previous_Co2_value == 0))
        {
          mqttclient.publish((String(MQTT_TOPIC) + "co2").c_str(), String(Co2).c_str());
        }
        if ((previous_TVOC_value != TVOC && TVOC != 0) || (TVOC == 0 && previous_TVOC_value == 0))
        {
          mqttclient.publish((String(MQTT_TOPIC) + "tvoc").c_str(), String(TVOC).c_str());
        }
        if ((previous_temp_value != temp && temp != 0) || (temp == 0 && previous_temp_value == 0))
        {
          mqttclient.publish((String(MQTT_TOPIC) + "temp").c_str(), String(temp).c_str());
        }
        if ((previous_hum_value != hum && hum != 0) || (hum == 0 && previous_hum_value == 0))
        {
          mqttclient.publish((String(MQTT_TOPIC) + "hum").c_str(), String(hum).c_str());
        }
      }
      else
      {
        connectToWifi();
        updateOLED();
      }
    }
  }
}
