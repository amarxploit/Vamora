
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define IR_SENSOR_PIN D5

ESP8266WebServer server(80);
WiFiClientSecure client;
UniversalTelegramBot bot("7720517434:AAHcN2UXx3GSX1Ly4kwVJCqScKICY4i6eHc", client);

// Default credentials
String ssid = "";
String password = "";
String telegramChatId = "";

// Flags
bool eyesWereOpen = true;

// HTML page for WiFi + Telegram config
String getHTML() {
  return "<html><body><h2>ESP8266 Config Panel</h2><form method='POST'>"
         "WiFi SSID:<br><input name='ssid'><br>"
         "WiFi Password:<br><input name='password' type='password'><br>"
         "Telegram Chat ID:<br><input name='chatid'><br><br>"
         "<input type='submit'></form></body></html>";
}

// Save config to SPIFFS
void saveConfig() {
  File f = SPIFFS.open("/config.txt", "w");
  if (f) {
    f.println(ssid);
    f.println(password);
    f.println(telegramChatId);
    f.close();
  }
}

// Load config from SPIFFS
void loadConfig() {
  File f = SPIFFS.open("/config.txt", "r");
  if (f) {
    ssid = f.readStringUntil('\n'); ssid.trim();
    password = f.readStringUntil('\n'); password.trim();
    telegramChatId = f.readStringUntil('\n'); telegramChatId.trim();
    f.close();
  }
}

// Handle form submission
void handleRoot() {
  if (server.method() == HTTP_POST) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    telegramChatId = server.arg("chatid");
    saveConfig();
    server.send(200, "text/html", "<h3>Saved. Reboot device.</h3>");
    delay(3000);
    ESP.restart();
  } else {
    server.send(200, "text/html", getHTML());
  }
}

// Set up web panel at 192.168.77.77
void startWebPortal() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,77,77), IPAddress(192,168,77,1), IPAddress(255,255,255,0));
  WiFi.softAP("ESP8266-CONFIG");
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web portal started at 192.168.77.77");
}

// Connect to saved WiFi
bool connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Connecting to WiFi");
  for (int i = 0; i < 20; i++) {
    if (WiFi.status() == WL_CONNECTED) return true;
    delay(500);
    Serial.print(".");
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  pinMode(IR_SENSOR_PIN, INPUT);
  SPIFFS.begin();
  loadConfig();

  if (!connectToWiFi()) {
    Serial.println("\nWiFi failed. Starting config portal.");
    startWebPortal();
  } else {
    Serial.println("\nWiFi connected!");
    client.setInsecure();  // Accept all SSL certs
  }
}

void loop() {
  if (WiFi.getMode() == WIFI_AP) {
    server.handleClient();
    return;
  }

  // IR sensor logic (unchanged)
  bool eyesClosed = digitalRead(IR_SENSOR_PIN) == LOW;
  if (eyesClosed && eyesWereOpen) {
    if (telegramChatId.length() > 0) {
      String msg = "😴 Eyes closed detected!";
      if (bot.sendMessage(telegramChatId, msg, "")) {
        Serial.println("Telegram message sent.");
      } else {
        Serial.println("Telegram send failed.");
      }
    }
    eyesWereOpen = false;
  } else if (!eyesClosed) {
    eyesWereOpen = true;
  }

  delay(1000);
}
