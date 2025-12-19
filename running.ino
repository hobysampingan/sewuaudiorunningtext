/*
 * SEWU AUDIO - Pure Running Text Display System v2.1
 * NodeMCU ESP8266 + LED P10
 * 
 * FITUR: 
 * - 5 Running Text dengan ON/OFF individual
 * - Separator antar teks
 * - Kecepatan scroll adjustable
 * - Font size adjustable
 * 
 * TANPA RTC - Murni Running Text Only
 * 
Pin on  DMD P10     GPIO      NODEMCU
        2  A        GPIO16    D0
        4  B        GPIO12    D6
        8  CLK      GPIO14    D5
        10 SCK      GPIO0     D3
        12 R        GPIO13    D7
        1  NOE      GPIO15    D8
        3  GND      GND       GND

SEWU AUDIO - Sound System & Lighting
*/

#include <SPI.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#include <HJS589.h>

#include <fonts/ElektronMart6x8.h>
#include <fonts/ElektronMart6x16.h>
#include <fonts/ElektronMart5x6.h>

#include "sewuwebpage.h"

// Data Structures
struct ConfigInfo {
  char nama[64];
  char info1[200];
  char info2[200];
  char info3[200];
  char info4[200];
  char info5[200];
  uint8_t enable1;
  uint8_t enable2;
  uint8_t enable3;
  uint8_t enable4;
  uint8_t enable5;
};

struct ConfigWifi {
  char wifissid[64];
  char wifipass[64];
};

struct ConfigDisp {
  int cerah;
  int panelCount;
  int speed;
  int fontSize;
  int separator;  // 0=none, 1=star, 2=diamond, 3=line, 4=blank
};

// LED Internal
uint8_t pin_led = 2;

// SETUP DMD
HJS589 *Disp = nullptr;

// WEB Server & DNS
ESP8266WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

IPAddress local_ip(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress netmask(255, 255, 255, 0);

const char *fileconfigdisp = "/configdisp.json";
ConfigDisp configdisp;

const char *fileconfiginfo = "/configinfo.json";
ConfigInfo configinfo;

const char *fileconfigwifi = "/configwifi.json";
ConfigWifi configwifi;

//----------------------------------------------------------------------
// FORWARD DECLARATIONS

void ICACHE_RAM_ATTR refresh();
void textCenter(int y, String Msg);
void branding();
void checkWiFiAP();
void restartWiFiAP();
void runText();
void showSeparator();
int getNextActiveText(int current);

// WiFi Watchdog
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000;
unsigned long wifiDownSince = 0;
bool wifiWasDown = false;

// Display state
uint8_t displayState = 0;  // 0=text, 1=separator
uint8_t runningTextMode = 0;
uint32_t scrollX = 0;
uint32_t lastScrollTime = 0;
uint32_t separatorStart = 0;

//----------------------------------------------------------------------
// XML DATA BRIDGE

void buildXML(){
  yield();

  XML = "<?xml version='1.0'?>";
  XML += "<rData>";
  
  XML += "<rNama>"; XML += configinfo.nama; XML += "</rNama>";
  XML += "<rInfo1>"; XML += configinfo.info1; XML += "</rInfo1>";
  XML += "<rInfo2>"; XML += configinfo.info2; XML += "</rInfo2>";
  XML += "<rInfo3>"; XML += configinfo.info3; XML += "</rInfo3>";
  XML += "<rInfo4>"; XML += configinfo.info4; XML += "</rInfo4>";
  XML += "<rInfo5>"; XML += configinfo.info5; XML += "</rInfo5>";
  XML += "<rEnable1>"; XML += configinfo.enable1; XML += "</rEnable1>";
  XML += "<rEnable2>"; XML += configinfo.enable2; XML += "</rEnable2>";
  XML += "<rEnable3>"; XML += configinfo.enable3; XML += "</rEnable3>";
  XML += "<rEnable4>"; XML += configinfo.enable4; XML += "</rEnable4>";
  XML += "<rEnable5>"; XML += configinfo.enable5; XML += "</rEnable5>";
  XML += "<rWifissid>"; XML += configwifi.wifissid; XML += "</rWifissid>";
  XML += "<rWifipass>"; XML += configwifi.wifipass; XML += "</rWifipass>";
  XML += "<rCerah>"; XML += configdisp.cerah; XML += "</rCerah>";
  XML += "<rPanelCount>"; XML += configdisp.panelCount; XML += "</rPanelCount>";
  XML += "<rSpeed>"; XML += configdisp.speed; XML += "</rSpeed>";
  XML += "<rFontSize>"; XML += configdisp.fontSize; XML += "</rFontSize>";
  XML += "<rSeparator>"; XML += configdisp.separator; XML += "</rSeparator>";

  XML += "</rData>";
  yield();
}

void handleXML() {
  yield();
  buildXML();
  server.send(200, "text/xml", XML);
  yield();
}

//----------------------------------------------------------------------
// SAFE PAGE SENDING

void sendPageSafe(const char* page) {
  yield();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  
  const size_t chunkSize = 1024;
  size_t len = strlen_P(page);
  size_t sent = 0;
  
  while (sent < len) {
    yield();
    size_t toSend = min(chunkSize, len - sent);
    char buffer[chunkSize + 1];
    memcpy_P(buffer, page + sent, toSend);
    buffer[toSend] = 0;
    server.sendContent(buffer);
    sent += toSend;
  }
  
  server.sendContent("");
  yield();
}

//----------------------------------------------------------------------
// REINIT DISPLAY

void Disp_reinit() {
  if (Disp != nullptr) {
    timer0_detachInterrupt();
    delete Disp;
  }
  
  Disp = new HJS589(configdisp.panelCount, 1);
  Disp->start();
  
  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(refresh);
  timer0_write(ESP.getCycleCount() + 80000);
  interrupts();
  
  Disp->clear();
  Disp->setBrightness(configdisp.cerah);
}

void ICACHE_RAM_ATTR refresh() {
  if (Disp != nullptr) {
    Disp->refresh();
    timer0_write(ESP.getCycleCount() + 80000);
  }
}

//----------------------------------------------------------------------
// LOAD DEFAULT DATA

void LoadDataAwal() {
  if (strlen(configinfo.nama) == 0) {
    strlcpy(configinfo.nama, "SEWU AUDIO", sizeof(configinfo.nama));
  }
  if (strlen(configinfo.info1) == 0) {
    strlcpy(configinfo.info1, "SELAMAT DATANG DI HAJATAN KAMI", sizeof(configinfo.info1));
  }
  if (strlen(configinfo.info2) == 0) {
    strlcpy(configinfo.info2, "SELAMAT MENIKMATI HIDANGAN", sizeof(configinfo.info2));
  }
  if (strlen(configinfo.info3) == 0) {
    strlcpy(configinfo.info3, "TERIMA KASIH ATAS KEHADIRANNYA", sizeof(configinfo.info3));
  }
  if (strlen(configinfo.info4) == 0) {
    strlcpy(configinfo.info4, "SEWU AUDIO SOUND SYSTEM", sizeof(configinfo.info4));
  }
  if (strlen(configinfo.info5) == 0) {
    strlcpy(configinfo.info5, "PROFESSIONAL LIGHTING SERVICE", sizeof(configinfo.info5));
  }

  // Default enable all
  if (configinfo.enable1 == 0 && configinfo.enable2 == 0 && 
      configinfo.enable3 == 0 && configinfo.enable4 == 0 && configinfo.enable5 == 0) {
    configinfo.enable1 = 1;
    configinfo.enable2 = 1;
    configinfo.enable3 = 1;
    configinfo.enable4 = 1;
    configinfo.enable5 = 1;
  }

  if (strlen(configwifi.wifissid) == 0) {
    strlcpy(configwifi.wifissid, "SEWU AUDIO", sizeof(configwifi.wifissid));
  }
  if (strlen(configwifi.wifipass) == 0) {
    strlcpy(configwifi.wifipass, "sewuaudio123", sizeof(configwifi.wifipass));
  }
  if (configdisp.cerah == 0) configdisp.cerah = 150;
  if (configdisp.panelCount == 0) configdisp.panelCount = 2;
  if (configdisp.speed == 0) configdisp.speed = 35;
  // separator default 0 (none)
}

//----------------------------------------------------------------------
// SETUP

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nSEWU AUDIO - Running Text v2.1");
  Serial.println("================================");

  pinMode(pin_led, OUTPUT);
  SPIFFS.begin();
  
  loadWifiConfig(fileconfigwifi, configwifi);
  loadInfoConfig(fileconfiginfo, configinfo);
  loadDispConfig(fileconfigdisp, configdisp);
  LoadDataAwal();

  // WiFi AP Mode
  WiFi.persistent(true);
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  delay(100);
  
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  
  WiFi.softAPConfig(local_ip, gateway, netmask);
  delay(100);
  
  int apRetry = 0;
  bool apStarted = false;
  while (!apStarted && apRetry < 5) {
    apStarted = WiFi.softAP(configwifi.wifissid, configwifi.wifipass, 1, false, 4);
    if (!apStarted) {
      Serial.print("AP retry "); Serial.println(apRetry + 1);
      delay(500);
      apRetry++;
    }
  }
  
  if (apStarted) {
    Serial.println("WiFi AP OK!");
    digitalWrite(pin_led, LOW);
  } else {
    digitalWrite(pin_led, HIGH);
  }
  
  WiFi.setOutputPower(20.5);
  dnsServer.start(DNS_PORT, "*", local_ip);
  
  Serial.print("SSID: "); Serial.println(configwifi.wifissid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  // Web Server
  server.on("/", []() { yield(); sendPageSafe(webpage); });
  server.on("/settingwifi", HTTP_POST, handleSettingWifiUpdate);
  server.on("/settinginfo", HTTP_POST, handleSettingInfoUpdate);
  server.on("/settingdisp", HTTP_POST, handleSettingDispUpdate);
  server.on("/xml", handleXML);
  server.begin();
  Serial.println("HTTP OK");

  Disp_reinit();
  branding();
  
  Serial.println("=== READY ===");
}

//----------------------------------------------------------------------
// CONFIG HANDLERS

void loadDispConfig(const char *fileconfigdisp, ConfigDisp &configdisp) {
  File configFileDisp = SPIFFS.open(fileconfigdisp, "r");
  if (!configFileDisp) return;

  size_t size = configFileDisp.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFileDisp.readBytes(buf.get(), size);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) return;
  
  configdisp.cerah = doc["cerah"];
  configdisp.panelCount = doc["panelCount"] | 2;
  configdisp.speed = doc["speed"] | 35;
  configdisp.fontSize = doc["fontSize"] | 0;
  configdisp.separator = doc["separator"] | 0;

  configFileDisp.close();
}

void handleSettingDispUpdate() {
  timer0_detachInterrupt();
  
  String datadisp = server.arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, datadisp);

  File configFileDisp = SPIFFS.open(fileconfigdisp, "w");
  if (!configFileDisp) {
    server.send(500, "application/json", "{\"status\":\"error\"}");
    timer0_attachInterrupt(refresh);
    return;
  }
  
  serializeJson(doc, configFileDisp);
  configFileDisp.close();
  
  if (!error) {
    loadDispConfig(fileconfigdisp, configdisp);
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    yield();
    Disp_reinit();
  } else {
    server.send(500, "application/json", "{\"status\":\"error\"}");
  }
  timer0_attachInterrupt(refresh);
}

void loadInfoConfig(const char *fileconfiginfo, ConfigInfo &configinfo) {
  File configFileInfo = SPIFFS.open(fileconfiginfo, "r");
  if (!configFileInfo) return;

  size_t size = configFileInfo.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFileInfo.readBytes(buf.get(), size);

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) return;

  strlcpy(configinfo.nama, doc["nama"] | "SEWU AUDIO", sizeof(configinfo.nama));
  strlcpy(configinfo.info1, doc["info1"] | "", sizeof(configinfo.info1));
  strlcpy(configinfo.info2, doc["info2"] | "", sizeof(configinfo.info2));
  strlcpy(configinfo.info3, doc["info3"] | "", sizeof(configinfo.info3));
  strlcpy(configinfo.info4, doc["info4"] | "", sizeof(configinfo.info4));
  strlcpy(configinfo.info5, doc["info5"] | "", sizeof(configinfo.info5));
  configinfo.enable1 = doc["enable1"] | 1;
  configinfo.enable2 = doc["enable2"] | 1;
  configinfo.enable3 = doc["enable3"] | 1;
  configinfo.enable4 = doc["enable4"] | 1;
  configinfo.enable5 = doc["enable5"] | 1;

  configFileInfo.close();
}

void handleSettingInfoUpdate() {
  timer0_detachInterrupt();
  yield();

  String datainfo = server.arg("plain");
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, datainfo);

  File configFileInfo = SPIFFS.open(fileconfiginfo, "w");
  if (!configFileInfo) {
    server.send(500, "application/json", "{\"status\":\"error\"}");
    timer0_attachInterrupt(refresh);
    return;
  }
  
  serializeJson(doc, configFileInfo);
  configFileInfo.close();
  
  if (!error) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    loadInfoConfig(fileconfiginfo, configinfo);
    yield();
    Disp_reinit();
  } else {
    server.send(500, "application/json", "{\"status\":\"error\"}");
  }
  timer0_attachInterrupt(refresh);
}

void loadWifiConfig(const char *fileconfigwifi, ConfigWifi &configwifi) {
  File configFileWifi = SPIFFS.open(fileconfigwifi, "r");
  if (!configFileWifi) return;

  size_t size = configFileWifi.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFileWifi.readBytes(buf.get(), size);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) return;

  strlcpy(configwifi.wifissid, doc["wifissid"] | "SEWU AUDIO", sizeof(configwifi.wifissid));
  strlcpy(configwifi.wifipass, doc["wifipass"] | "sewuaudio123", sizeof(configwifi.wifipass));

  configFileWifi.close();
}

void handleSettingWifiUpdate() {
  timer0_detachInterrupt();
  yield();
  
  String data = server.arg("plain");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, data);

  File configFile = SPIFFS.open("/configwifi.json", "w");
  if (!configFile) {
    server.send(500, "application/json", "{\"status\":\"error\"}");
    timer0_attachInterrupt(refresh);
    return;
  }
  
  serializeJson(doc, configFile);
  configFile.close();
  
  if (!error) {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    loadWifiConfig(fileconfigwifi, configwifi);
    yield();
    delay(1000);
    ESP.restart();
  } else {
    server.send(500, "application/json", "{\"status\":\"error\"}");
  }
  timer0_attachInterrupt(refresh);
}

//----------------------------------------------------------------------
// LOOP

void loop() {
  yield();
  dnsServer.processNextRequest();
  server.handleClient();
  yield();
  checkWiFiAP();
  yield();

  // State machine: text atau separator
  if (displayState == 0) {
    runText();
  } else {
    showSeparator();
  }
  
  yield();
}

//----------------------------------------------------------------------
// GET NEXT ACTIVE TEXT

int getNextActiveText(int current) {
  int next = current + 1;
  int attempts = 0;
  
  while (attempts < 7) {
    if (next > 5) next = 0;
    
    switch(next) {
      case 0: return 0;  // Nama selalu aktif
      case 1: if (configinfo.enable1) return 1; break;
      case 2: if (configinfo.enable2) return 2; break;
      case 3: if (configinfo.enable3) return 3; break;
      case 4: if (configinfo.enable4) return 4; break;
      case 5: if (configinfo.enable5) return 5; break;
    }
    next++;
    attempts++;
  }
  return 0;
}

//----------------------------------------------------------------------
// SEPARATOR DISPLAY

void showSeparator() {
  uint32_t currentTime = millis();
  int width = Disp->width();
  
  // Separator duration
  if (configdisp.separator == 4) {
    // Blank - just pause
    if (currentTime - separatorStart >= 500) {
      displayState = 0;
      Disp->clear();
    }
  } else if (configdisp.separator > 0) {
    // Scrolling separator
    static uint32_t sepX = 0;
    static char sepText[32];
    
    if (sepX == 0) {
      // Set separator text
      switch(configdisp.separator) {
        case 1: strcpy(sepText, "* * * * *"); break;
        case 2: strcpy(sepText, "+ + + + +"); break;
        case 3: strcpy(sepText, "- - - - -"); break;
        default: strcpy(sepText, "* * *"); break;
      }
    }
    
    Disp->setFont(ElektronMart6x8);
    int sepWidth = Disp->textWidth(sepText);
    int fullScroll = sepWidth + width;
    
    if (currentTime - lastScrollTime >= configdisp.speed) {
      lastScrollTime = currentTime;
      
      if (sepX < fullScroll) {
        sepX++;
        int yPos = (configdisp.fontSize == 1) ? 4 : 4;
        Disp->drawText(width - sepX, yPos, sepText);
      } else {
        sepX = 0;
        displayState = 0;
        Disp->clear();
      }
    }
  } else {
    // No separator
    displayState = 0;
  }
}

//----------------------------------------------------------------------
// RUNNING TEXT

void runText() {
  uint32_t currentTime = millis();
  int width = Disp->width();
  
  char* currentText;
  static char namaWithPrefix[128];
  
  switch(runningTextMode) {
    case 0: 
      snprintf(namaWithPrefix, sizeof(namaWithPrefix), "* %s *", configinfo.nama);
      currentText = namaWithPrefix;
      break;
    case 1: currentText = configinfo.info1; break;
    case 2: currentText = configinfo.info2; break;
    case 3: currentText = configinfo.info3; break;
    case 4: currentText = configinfo.info4; break;
    case 5: currentText = configinfo.info5; break;
    default: currentText = configinfo.nama; break;
  }
  
  if (configdisp.fontSize == 1) {
    Disp->setFont(ElektronMart6x16);
  } else {
    Disp->setFont(ElektronMart6x8);
  }
  
  int textWidth = Disp->textWidth(currentText);
  int fullScroll = textWidth + width;
  
  if (currentTime - lastScrollTime >= configdisp.speed) {
    lastScrollTime = currentTime;
    
    if (scrollX < fullScroll) {
      scrollX++;
    } else {
      scrollX = 0;
      Disp->clear();
      
      // Move to next text
      runningTextMode = getNextActiveText(runningTextMode);
      
      // Show separator if enabled
      if (configdisp.separator > 0) {
        displayState = 1;
        separatorStart = millis();
      }
      return;
    }
    
    int yPos = (configdisp.fontSize == 1) ? 0 : 4;
    Disp->drawText(width - scrollX, yPos, currentText);
  }
}

//----------------------------------------------------------------------
// UTILITIES

void textCenter(int y, String Msg) {
  int center = int((Disp->width() - Disp->textWidth(Msg)) / 2);
  Disp->drawText(center, y, Msg);
}

void branding() {
  Disp->clear();
  Disp->setFont(ElektronMart6x8);
  textCenter(0, "SEWU");
  textCenter(8, "AUDIO");
  delay(2000);

  Disp->clear();
  Disp->setFont(ElektronMart5x6);
  textCenter(4, "READY!");
  delay(1000);
  Disp->clear();
}

//----------------------------------------------------------------------
// WIFI WATCHDOG

void checkWiFiAP() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastWiFiCheck < WIFI_CHECK_INTERVAL) return;
  lastWiFiCheck = currentMillis;
  
  WiFiMode_t currentMode = WiFi.getMode();
  IPAddress apIP = WiFi.softAPIP();
  
  bool wifiIsDown = (currentMode != WIFI_AP && currentMode != WIFI_AP_STA) || 
                    (apIP[0] == 0 && apIP[1] == 0 && apIP[2] == 0 && apIP[3] == 0);
  
  if (wifiIsDown) {
    if (!wifiWasDown) {
      wifiDownSince = currentMillis;
      wifiWasDown = true;
    } else if (currentMillis - wifiDownSince > 3000) {
      restartWiFiAP();
    }
    digitalWrite(pin_led, HIGH);
  } else {
    if (wifiWasDown) wifiWasDown = false;
    digitalWrite(pin_led, LOW);
  }
}

void restartWiFiAP() {
  dnsServer.stop();
  yield();
  
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  yield();
  delay(200);
  
  WiFi.mode(WIFI_OFF);
  yield();
  delay(200);
  
  WiFi.mode(WIFI_AP);
  yield();
  delay(100);
  
  WiFi.softAPConfig(local_ip, gateway, netmask);
  yield();
  delay(100);
  
  bool apStarted = WiFi.softAP(configwifi.wifissid, configwifi.wifipass, 1, false, 4);
  
  if (apStarted) {
    dnsServer.start(DNS_PORT, "*", local_ip);
    wifiWasDown = false;
    digitalWrite(pin_led, LOW);
  } else {
    digitalWrite(pin_led, HIGH);
  }
  yield();
}
