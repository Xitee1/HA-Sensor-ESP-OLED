#include <U8g2lib.h>

#include <ArduinoJson.h>
#include <time.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>


// ---------------- //
// --- Settings --- //
// ---------------- //

// WiFi
#include <ESP8266WiFi.h> // ESP8266 WiFi
//#include <WiFi.h> // ESP32 WiFi

#define WIFI_AP "SSID"
#define WIFI_PASSWORD "password"

// HomeAssistant API URL
#define HA_AUTH "Bearer PASTE_YOUR_TOKEN_HERE"
#define API_URL "http://homeassistant.local:8123/api/states/"

// Buttons
int backButton = 12;
int setButton = 13;
int nextButton = 14;

// NTP Server
#define NTP_SERVER "pool.ntp.org"
#define TIMEZONE "CET-1CEST,M3.5.0/02,M10.5.0/03"

// Display
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2 = U8G2_SH1106_128X64_NONAME_1_HW_I2C(U8G2_R0);

// Entities
const char* entities[] = {
                        "sensor.wohnzimmer_temperatur",
                        "sensor.draussen_temperatur",
                        "sensor.sonnenbatterie_state_production_w",
                        "sensor.sonnenbatterie_state_consumption_w",
                        "sensor.sonnenbatterie_state_charge_real",
                      };

// ---------------- //


// ------------------------ //
// --- Public Variables --- //
// ------------------------ //
int currentEntity = 0;
int entitySize = sizeof(entities) / sizeof(entities[0]) -1;

int displayBrightness = 3;


// 0 = System starting
// 1 = System info
// 2 = Entities
int currentPage = 0;

// Timers
long dataUpdateInterval = 1000*3; // Fetch new sensor data from HA all 10 seconds
unsigned long lastDataUpdate;

long displayUpdateInterval = 250; // Update display all 1000ms
unsigned long lastDisplayUpdate;

long timeUpdateInterval = 1000*60*60; // Time server update all 60 minutes
unsigned long lastTimeUpdate;

long buttonPressDelay = 350; // Timer to prevent button glitching
unsigned long lastButtonPress;


bool buttonPressed = false;

// Entity data
const char* e_name;
int e_name_offset;
int e_name_width;
String e_value;
//const char* e_value;
//const char* e_unit;


HTTPClient http;
WiFiClient wifiClient;

time_t now;
tm tm;

// ------------------- //
// --- The program --- //
// ------------------- //
void setup() {
  Serial.begin(9600);
  delay(100);

  // Initialize Display
  pinMode(4, OUTPUT);
  u8g2.begin();
  u8g2.enableUTF8Print();

  // Set pin modes
  pinMode(backButton, INPUT_PULLUP);
  pinMode(setButton, INPUT_PULLUP);
  pinMode(nextButton, INPUT_PULLUP);

  // Draw info on display, that the system is starting
  updateDisplay();

  // Connect to WiFi and get current time
  connectWiFi();
  updateTime();

  updateData(true);
  updateDisplay();

  currentPage = 2;

  delay(500);
}

void loop() {
  // Check for pressed buttons
  if(digitalRead(backButton) == LOW) {
    // This prevents that when the button is holded for a second that it doesn't go wild and executed this code thousand times
    if(!buttonPressed) {
      lastButtonPress = millis();
      buttonPressed = true;

      // Go one sensor back
      if(currentEntity == 0) {
        currentEntity = entitySize;
        scroll_delay = 6; // 4 = ~1 second - when display refresh all 250ms
      } else {
        currentEntity -= 1;
      }

      updateData(true);
      updateDisplay();
    }
  }else

  if(digitalRead(nextButton) == LOW) {
    // This prevents that when the button is holded for a second that it doesn't go wild and executed this code thousand times
    if(buttonPressed) {
      lastButtonPress = millis();
      buttonPressed = true;

      // Go one sensor forward
      if(currentEntity == entitySize) {
        currentEntity = 0;
        scroll_delay = 6; // 4 = ~1 second - when display refresh all 250ms
      } else {
        currentEntity += 1;
      }

      updateData(true);
      updateDisplay();
    }

  }else

  if(digitalRead(setButton) == LOW) {
    // This prevents that when the button is holded for a second that it doesn't go wild and executed this code thousand times
    if(buttonPressed) {
      lastButtonPress = millis();
      buttonPressed = true;

      // Single-Press: Change brightness
      /*
      if(displayBrightness == 15) { // 7 = max brightness
        setSSD1306VcomDeselect(2);
        setSSD1306PreChargePeriod(15, 1);
        displayBrightness = 0;
      }else {
        setSSD1306VcomDeselect(displayBrightness);
        setSSD1306PreChargePeriod(15, 1);
        displayBrightness += 1;
      }*/
      
      if(displayBrightness == 1) {
        displayBrightness = 0;
        u8g2.clear();
      } else {
        displayBrightness = 1;
        updateData(true);
        updateDisplay();
      }
    }
    
  }else if(millis() - lastButtonPress > buttonPressDelay) {
    buttonPressed = false;
  }


  // Timers

  // Data from HA
  if(displayBrightness != 0) {
    if(millis() - lastDataUpdate > dataUpdateInterval) {
      lastDataUpdate = millis();
      updateData(false);
    }
  }

  // Display
  if(millis() - lastDisplayUpdate > displayUpdateInterval) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
  // Time
  if(millis() - lastTimeUpdate > timeUpdateInterval) {
    lastTimeUpdate = millis();
    updateTime();
  }
}


void connectWiFi() {
  Serial.println("Connecting to WiFi:");

  Serial.print("SSID: ");
  Serial.println(WIFI_AP);
  Serial.print("Password: ");
  Serial.println(WIFI_PASSWORD);

  Serial.println("Connecting..");

  updateDisplay();
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED && WiFi.status() != WL_WRONG_PASSWORD && WiFi.status() != WL_NO_SSID_AVAIL && WiFi.status() != WL_CONNECT_FAILED) {
    delay(500);
    Serial.print(".");
    updateDisplay();
  }
  updateDisplay();
  Serial.println("WiFi-Status (3 = connected): "+WiFi.status());
}
void updateTime() {
  configTime(0, 0, NTP_SERVER);  
  setenv("TZ", TIMEZONE, 0);
}

void updateData(bool first) {
  String url = String(API_URL)+entities[currentEntity];
  Serial.print("Fetching data from URL: ");
  Serial.println(url);
  http.begin(wifiClient, url);
  http.addHeader("Authorization", HA_AUTH);
  http.addHeader("Content-Type", "application/json");
  int statuscode = http.GET();
  if(statuscode != 200) {
    u8g2.firstPage();
    
    //char statustext[5];
    //snprintf(statustext, sizeof(statuscode), "%i", statuscode);

    do {
      u8g2.setFont(u8g2_font_profont17_mf);
      u8g2.drawStr(0, 11, "No data");
      
      u8g2.setFont(u8g2_font_profont10_mf);
      u8g2.drawStr(0, 23, "Could not get Data");
      u8g2.drawStr(0, 33, "from Home Assistant.");
      u8g2.drawStr(0, 43, "Error code:");
      u8g2.setCursor(0, 53);
      u8g2.print(statuscode);
      //u8g2.drawStr(0, 53, statustext);
    } while (u8g2.nextPage());
    http.end();
  }

  DynamicJsonDocument doc(2048);
  deserializeJson(doc, http.getString());
  http.end();
  
  // Get the state
  const char* state = doc["state"].as<char*>();
  Serial.print("Value: ");
  Serial.println(e_value);



  DynamicJsonDocument doc_a(2048);
  deserializeJson(doc_a, doc["attributes"].as<String>());

  e_name = doc_a["friendly_name"].as<char*>();
  const char* unit = doc_a["unit_of_measurement"].as<char*>();

  e_value = String(state)+unit;

  // Only get attributes on first update since they normally do not change
  if(first == true) {
    


    // Get name width
    u8g2.setFont(u8g2_font_profont17_mf);
    e_name_width = u8g2.getUTF8Width(e_name) + 30;
    e_name_offset = 0;
    scroll_delay = 6;
  }

}

/*
// value from 0 to 7, higher values more brighter
void setSSD1306VcomDeselect(uint8_t v)
{	
  u8g2.sendF("cac", 0x0db, v << 4, 0xaf);
}

// p1: 1..15, higher values, more darker, however almost no difference with 3 or more
// p2: 1..15, higher values, more brighter
void setSSD1306PreChargePeriod(uint8_t p1, uint8_t p2)
{	
  u8g2.sendF("ca", 0x0d9, (p2 << 4) | p1 );
}
*/
// ---------------- //
// --- Displays --- //
// ---------------- //
void updateDisplay() {
  if(displayBrightness == 0)
    return;



  if(currentPage == 0) {
    drawSystemStarting();
    return;
  }
  if(currentPage == 1) {
    //drawSystemInfo();
    return;
  }
  if(currentPage == 2) {

    drawEntity(currentEntity);
  }



}

int scroll_delay = 0;
void drawEntity(int currentEntity) {

  

  // Calculate things for the display
  u8g2.setFont(u8g2_font_logisoso26_tf);
  const int stringWidth = u8g2.getUTF8Width(e_value.c_str());
  const int textspace = 1;
  const int boxstart = int((u8g2.getDisplayWidth() - stringWidth) / 2);
  const int boxlength = stringWidth + (textspace * 2);
  

  time(&now);
  localtime_r(&now, &tm);

  char date[11];
  snprintf(date, 11, "%02d.%02d.%02d", tm.tm_mday, tm.tm_mon+1, (tm.tm_year+1900) % 100); // % 100 formats the date from 2022 to 22

  char time[9];
  snprintf(time, 9, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);


  // Get time start pixel
  u8g2.setFont(u8g2_font_profont15_mf); // Set font so the size gets calculated correctly
  const int timeStart = u8g2.getDisplayWidth() - u8g2.getStrWidth(time);

  u8g2_uint_t x;

  u8g2.firstPage();
  do {
    // Draw entity name
    u8g2.setFont(u8g2_font_profont17_mf);
    
    if(e_name_width-30 > u8g2.getDisplayWidth()) { // Scroll text if text is longer than the screen
      x = e_name_offset;
      do {
        u8g2.drawUTF8(x, 11, e_name);
        x += e_name_width;
      } while(x < u8g2.getDisplayWidth());
    } else {
      u8g2.drawUTF8(0, 11, e_name);
    }


    // Draw date and time
    u8g2.setFont(u8g2_font_profont15_mf);
    u8g2.drawStr(0, 25, date);
    u8g2.drawStr(timeStart, 25, time);

    // Draw box
    u8g2.drawRFrame(boxstart, 28, boxlength, 36, 10);
    
    // Draw value
    u8g2.setFont(u8g2_font_logisoso26_tf);
    u8g2.drawUTF8(boxstart + textspace, 60, e_value.c_str());

  } while (u8g2.nextPage());



  // entity name scroll text
  if(e_name_width-30 > u8g2.getDisplayWidth()) {
    if(scroll_delay <= 0) {
      e_name_offset -= 10;
      if((u8g2_uint_t) e_name_offset < (u8g2_uint_t) - e_name_width) {
        e_name_offset = 0;
        scroll_delay = 6; // 4 = ~1 second - when display refresh all 250ms
      }
    }else {
      scroll_delay -= 1;
    }
  }
}


void drawGettingData() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont17_mf);
    u8g2.drawStr(0, 11, "Loading data");
    
    u8g2.setFont(u8g2_font_profont10_mf);
    u8g2.drawStr(0, 23, "Loading data from");
    u8g2.drawStr(0, 33, "Home Assistant.");
    u8g2.drawStr(0, 43, "");
    u8g2.drawStr(0, 53, "");
  } while (u8g2.nextPage());
}

void drawSystemStarting() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont17_mf);
    u8g2.drawStr(0, 11, "Starting..");
      
    u8g2.setFont(u8g2_font_profont10_mf);
    u8g2.setCursor(0, 23);
    u8g2.print("WiFi SSID: ");
    u8g2.print(WIFI_AP);
    u8g2.setCursor(0, 33);
    u8g2.print("HA: ");
    u8g2.print(API_URL);

    // WiFi
    u8g2.setCursor(0, 43);
    int wifiState = WiFi.status();
    const char* wifiStateName = "Unknown";
    if(wifiState == 0)
      wifiStateName = "None";
    if(wifiState == 1)
      wifiStateName = "SSID not reachable";
    if(wifiState == 3)
      wifiStateName = "Connected";
    if(wifiState == 4)
      wifiStateName = "Connection failed";
    if(wifiState == 6)
      wifiStateName = "Wrong password";
    if(wifiState == 7)
      wifiStateName = "Connecting..";
    
    u8g2.print("WiFi: ");
    u8g2.print(wifiState);
    u8g2.print(" - ");
    u8g2.print(wifiStateName);
    
  } while (u8g2.nextPage());
}
