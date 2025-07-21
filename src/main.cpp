/*
  -------------------------------------------------------------------------
  ESP32 Improv WiFi + Antenna Control - Direct SDK Implementation
  -------------------------------------------------------------------------
  Uses the direct improv.h and improv.cpp files (official SDK)
  Based on Grey Lancaster's successful working implementation
  -------------------------------------------------------------------------
*/

// Define LED for ESP32
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Antenna control (comment out for regular ESP32)
#define ANTENNA_CTRL_GPIO3 3
#define ANTENNA_CTRL_GPIO14 14
bool useExternalAntenna = false;

#include <WiFi.h>
#include <Esp.h>
#include "improv.h"  // Use your direct SDK files

WiFiServer server(80);

char linebuf[80];
int charcount = 0;

// Improv Serial handling
uint8_t buffer[128];
size_t position = 0;
bool wifiConnected = false;

// Forward declarations
void handleHttpRequest();

void setupAntenna() {
  // Comment out for regular ESP32 testing
  // pinMode(ANTENNA_CTRL_GPIO3, OUTPUT);
  // pinMode(ANTENNA_CTRL_GPIO14, OUTPUT);
  Serial.println("✓ Antenna control disabled for ESP32 testing");
}

void setAntenna(bool external) {
  useExternalAntenna = external;
  
  if (useExternalAntenna) {
    // For ESP32-C6: GPIO3 LOW, GPIO14 HIGH
    // digitalWrite(ANTENNA_CTRL_GPIO3, LOW);
    // digitalWrite(ANTENNA_CTRL_GPIO14, HIGH);
    Serial.println("✓ External antenna (simulated)");
  } else {
    // For ESP32-C6: GPIO3 HIGH, GPIO14 LOW
    // digitalWrite(ANTENNA_CTRL_GPIO3, HIGH);
    // digitalWrite(ANTENNA_CTRL_GPIO14, LOW);
    Serial.println("✓ Onboard antenna");
  }
  
  delay(100);
}

void blink_led(unsigned long delayMs, int times) {
  pinMode(LED_BUILTIN, OUTPUT);
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(delayMs);
  }
}

void sendImprovState(improv::State state) {
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.resize(11);
  data[6] = improv::IMPROV_SERIAL_VERSION;
  data[7] = improv::TYPE_CURRENT_STATE;
  data[8] = 1;
  data[9] = state;
  
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < data.size() - 1; i++) {
    checksum += data[i];
  }
  data[10] = checksum;
  
  Serial.write(data.data(), data.size());
}

void sendImprovInfoResponse() {
  std::vector<std::string> info = {
    "ImprovWiFiLib",         // Firmware name (same as working version)
    "1.0.0",                 // Firmware version  
    "ESP32",                 // Hardware chip/variant
    "BasicWebServer"         // Device name (same as working version)
  };
  
  std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
  data.push_back(improv::IMPROV_SERIAL_VERSION);
  data.push_back(improv::TYPE_RPC_RESPONSE);
  
  auto response = improv::build_rpc_response(improv::GET_DEVICE_INFO, info, false);
  data.insert(data.end(), response.begin(), response.end());
  
  uint8_t checksum = 0;
  for (uint8_t byte : data) {
    checksum += byte;
  }
  data.push_back(checksum);
  
  Serial.write(data.data(), data.size());
}

bool connectWifi(const std::string& ssid, const std::string& password) {
  Serial.print("🔄 Connecting to WiFi: ");
  Serial.println(ssid.c_str());
  
  WiFi.begin((char*)ssid.c_str(), password.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✓ WiFi Connected!");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Send success response
    std::vector<std::string> response = {WiFi.localIP().toString().c_str()};
    std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
    data.push_back(improv::IMPROV_SERIAL_VERSION);
    data.push_back(improv::TYPE_RPC_RESPONSE);
    
    auto rpc_response = improv::build_rpc_response(improv::WIFI_SETTINGS, response, false);
    data.insert(data.end(), rpc_response.begin(), rpc_response.end());
    
    uint8_t checksum = 0;
    for (uint8_t byte : data) {
      checksum += byte;
    }
    data.push_back(checksum);
    
    Serial.write(data.data(), data.size());
    
    server.begin();
    wifiConnected = true;
    sendImprovState(improv::STATE_PROVISIONED);
    blink_led(100, 3);
    return true;
  } else {
    Serial.println();
    Serial.println("❌ WiFi connection failed!");
    
    // Send error response
    std::vector<uint8_t> data = {'I', 'M', 'P', 'R', 'O', 'V'};
    data.resize(11);
    data[6] = improv::IMPROV_SERIAL_VERSION;
    data[7] = improv::TYPE_ERROR_STATE;
    data[8] = 1;
    data[9] = improv::ERROR_UNABLE_TO_CONNECT;
    
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < data.size() - 1; i++) {
      checksum += data[i];
    }
    data[10] = checksum;
    
    Serial.write(data.data(), data.size());
    
    sendImprovState(improv::STATE_AUTHORIZED);
    blink_led(1000, 3);
    return false;
  }
}

bool onImprovCommandCallback(improv::ImprovCommand command) {
  switch (command.command) {
    case improv::WIFI_SETTINGS: {
      if (command.ssid.length() == 0) {
        Serial.println("❌ Invalid SSID received");
        return false;
      }
      
      sendImprovState(improv::STATE_PROVISIONING);
      return connectWifi(command.ssid, command.password);
    }
    
    case improv::GET_DEVICE_INFO: {
      sendImprovInfoResponse();
      return true;
    }
    
    case improv::IDENTIFY: {
      Serial.println("🔍 Identify command received");
      blink_led(100, 10);  // Rapid blinking for identification
      return true;
    }
    
    default:
      Serial.println("❌ Unknown Improv command");
      return false;
  }
}

void onImprovErrorCallback(improv::Error error) {
  Serial.print("❌ Improv Error: ");
  Serial.println(error);
  blink_led(2000, 3);
}

void handleImprovSerial() {
  while (Serial.available()) {
    uint8_t byte = Serial.read();
    
    if (position == 0 && byte != 'I') {
      continue;
    }
    
    buffer[position++] = byte;
    
    if (position >= sizeof(buffer)) {
      position = 0;
      continue;
    }
    
    if (improv::parse_improv_serial_byte(position - 1, byte, buffer, 
        [](improv::ImprovCommand command) -> bool {
          return onImprovCommandCallback(command);
        },
        [](improv::Error error) {
          onImprovErrorCallback(error);
        })) {
      position = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // Give serial time to initialize
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Initialize antenna control
  setupAntenna();

  Serial.println("🚀 ESP32 Improv WiFi + Antenna Control");
  Serial.println("   Using Direct Improv WiFi SDK");

  // More careful WiFi initialization
  WiFi.mode(WIFI_OFF);
  delay(500);
  
  WiFi.mode(WIFI_STA);
  delay(500);

  Serial.println("   Ready for Improv WiFi configuration");
  Serial.println("   Device: BasicWebServer (ImprovWiFiLib v1.0.0)");
  
  // Send initial state
  sendImprovState(improv::STATE_AUTHORIZED);
  
  blink_led(100, 5);
  Serial.println("Setup complete. Waiting for Improv WiFi commands...");
}

void loop() {
  handleImprovSerial();

  // Debug output every 10 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 10000) {
    Serial.println("--- Status: Waiting for Improv WiFi ---");
    Serial.println("WiFi Status: " + String(WiFi.status()));
    Serial.println("Connected: " + String(wifiConnected));
    lastDebug = millis();
  }

  if (wifiConnected && WiFi.status() == WL_CONNECTED) {
    handleHttpRequest();
  }
}

void handleHttpRequest() {
  WiFiClient client = server.available();
  if (client) {
    blink_led(50, 1);
    memset(linebuf, 0, sizeof(linebuf));
    charcount = 0;
    boolean currentLineIsBlank = true;
    
    String request = "";
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        linebuf[charcount] = c;
        if (charcount < sizeof(linebuf) - 1)
          charcount++;

        request += c;

        if (c == '\n' && currentLineIsBlank) {
          // Check for antenna switch requests
          if (request.indexOf("GET /antenna/external") >= 0) {
            Serial.println("🌐 Web request: Switch to external antenna");
            setAntenna(true);
            blink_led(100, 2);
          } else if (request.indexOf("GET /antenna/onboard") >= 0) {
            Serial.println("🌐 Web request: Switch to onboard antenna");
            setAntenna(false);
            blink_led(100, 2);
          } else if (request.indexOf("GET /api/rssi") >= 0) {
            // JSON API endpoint for RSSI
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println("Access-Control-Allow-Origin: *");
            client.println();
            client.print("{\"rssi\":");
            client.print(WiFi.RSSI());
            client.print(",\"antenna\":\"");
            client.print(useExternalAntenna ? "external" : "onboard");
            client.println("\"}");
            break;
          }
          
          // Serve main webpage
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close"); 
          client.println();
          client.println("<!DOCTYPE HTML><html><head>");
          client.println("<title>ESP32 Improv WiFi + Antenna Control</title>");
          client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<style>");
          client.println("body{font-family:Arial;margin:20px;background:#f0f0f0;}");
          client.println(".container{max-width:800px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}");
          client.println("h1{color:#333;text-align:center;margin-bottom:30px;}");
          client.println(".info-card{background:#f8f9fa;padding:15px;margin:15px 0;border-radius:8px;border-left:4px solid #007bff;}");
          client.println(".info-item{margin:8px 0;display:flex;justify-content:space-between;}");
          client.println(".info-label{font-weight:bold;color:#666;}");
          client.println(".info-value{color:#333;}");
          client.println(".antenna-control{background:#e8f4fd;border-left-color:#17a2b8;}");
          client.println("button{padding:12px 24px;border:none;border-radius:6px;cursor:pointer;font-weight:bold;margin:8px;font-size:16px;}");
          client.println(".btn-primary{background:#007bff;color:white;}");
          client.println(".btn-primary:hover{background:#0056b3;}");
          client.println(".btn-success{background:#28a745;color:white;}");
          client.println(".btn-success:hover{background:#1e7e34;}");
          client.println(".btn-active{background:#ffc107;color:#212529;box-shadow:0 0 0 3px rgba(255,193,7,0.25);}");
          client.println(".improv-badge{background:#6f42c1;color:white;padding:4px 8px;border-radius:4px;font-size:12px;font-weight:bold;}");
          client.println(".status-badge{background:#28a745;color:white;padding:4px 8px;border-radius:4px;font-size:12px;font-weight:bold;}");
          client.println("</style>");
          client.println("</head><body>");
          client.println("<div class=\"container\">");
          client.println("<h1>&#127760; ESP32 Improv WiFi + Antenna Control</h1>");
          
          // Network Info Card
          client.println("<div class=\"info-card\">");
          client.println("<h3>&#127760; Network Information <span class=\"improv-badge\">DIRECT SDK</span> <span class=\"status-badge\">ONLINE</span></h3>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">SSID:</span><span class=\"info-value\">");
          client.println(WiFi.SSID());
          client.println("</span></div>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">IP Address:</span><span class=\"info-value\">");
          client.println(WiFi.localIP());
          client.println("</span></div>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">MAC Address:</span><span class=\"info-value\">");
          
          // Get MAC address properly for ESP32
          uint8_t macAddr[6];
          WiFi.macAddress(macAddr);
          char macStr[18];
          sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                  macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
          client.println(macStr);
          
          client.println("</span></div>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">Signal Strength:</span><span class=\"info-value\" id=\"rssi-display\">");
          client.println(WiFi.RSSI());
          client.println(" dBm</span></div>");
          client.println("</div>");
          
          // Antenna Control Card
          client.println("<div class=\"info-card antenna-control\">");
          client.println("<h3>&#128225; Antenna Control</h3>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">Current Antenna:</span><span class=\"info-value\" id=\"current-antenna\">");
          client.println(useExternalAntenna ? "External" : "Onboard");
          client.println("</span></div>");
          client.println("<div style=\"text-align:center;margin:15px 0;\">");
          client.print("<button class=\"btn-primary");
          if (!useExternalAntenna) client.print(" btn-active");
          client.println("\" onclick=\"location.href='/antenna/onboard'\">&#128241; Onboard Antenna</button>");
          client.print("<button class=\"btn-success");
          if (useExternalAntenna) client.print(" btn-active");
          client.println("\" onclick=\"location.href='/antenna/external'\">&#128225; External Antenna</button>");
          client.println("</div>");
          client.println("<p style=\"text-align:center;color:#666;font-size:14px;\">&#9888;&#65039; Note: Antenna switching simulated on regular ESP32</p>");
          client.println("</div>");
          
          // Chip Info Card
          client.println("<div class=\"info-card\">");
          client.println("<h3>&#128187; Device Information</h3>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">Chip Model:</span><span class=\"info-value\">");
          client.println(ESP.getChipModel());
          client.println("</span></div>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">CPU Cores:</span><span class=\"info-value\">");
          client.println(ESP.getChipCores());
          client.println("</span></div>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">Chip Revision:</span><span class=\"info-value\">");
          client.println(ESP.getChipRevision());
          client.println("</span></div>");
          client.println("<div class=\"info-item\"><span class=\"info-label\">Free Heap:</span><span class=\"info-value\">");
          client.print(ESP.getFreeHeap());
          client.println(" bytes</span></div>");
          client.println("</div>");
          
          client.println("<p style=\"text-align:center;margin-top:30px;color:#666;\">");
          client.println("<small>Configured via Direct Improv WiFi SDK | ESP32 Antenna Control Demo</small>");
          client.println("</p>");
          
          // Auto-refresh RSSI
          client.println("<script>");
          client.println("function updateRSSI() {");
          client.println("  fetch('/api/rssi').then(r => r.json()).then(data => {");
          client.println("    document.getElementById('rssi-display').textContent = data.rssi + ' dBm';");
          client.println("    document.getElementById('current-antenna').textContent = data.antenna.charAt(0).toUpperCase() + data.antenna.slice(1);");
          client.println("  }).catch(e => console.log('RSSI update failed'));");
          client.println("}");
          client.println("setInterval(updateRSSI, 5000);");
          client.println("</script>");
          
          client.println("</div></body></html>");
          break;
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop();
  }
}