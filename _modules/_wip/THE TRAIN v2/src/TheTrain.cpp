#include <PowerFunctions.h>   // Power Functions Library
#include <CyberCitySharedFunctionality.h>     // Custom library for shared project functionality
#include <Arduino.h>                          // Core Arduino functions

// -------------------------------------------------------------------
// MODE SWITCH: Set to true for offline IR testing. Set to false for WiFi/MQTT.
#define DEBUG_MODE false
// -------------------------------------------------------------------

#if !DEBUG_MODE
  #include <WiFi.h>                             // WiFi connectivity library
  #include <PubSubClient.h>                     // MQTT client library
  #include "sensitiveInformation.h"             // Contains WiFi credentials and MQTT settings
  WiFiClient espClient;                         // WiFi client for MQTT communication
  PubSubClient client(espClient);   
#endif

// IR Channels
#define CH1 0x0
#define CH2 0x1
#define CH3 0x2
#define CH4 0x3

// IR Transmission Pin for Adafruit High Power IR Emitter Breakout (Adafruit #5639)
#define IR_TRANS_IN   21  
#define IR_DEBUG_OFF  0  
#define IR_DEBUG_ON   1  

// Call PowerFunctions parameters
PowerFunctions pf(IR_TRANS_IN, CH1, IR_DEBUG_ON);
CyberCitySharedFunctionality cyberCity;       // Instance of shared functionality class

// Timers for non-blocking execution
unsigned long lastDebugTime = 0;
const unsigned long debugInterval = 3000;     // Step through speeds every 3 seconds

String currentStatus = "STOPPED";
String lastDisplayedStatus = "";
String lastDisplayedSpeed = "";

// Single and dual motor control is defined
void step(uint8_t output, uint8_t pwm, uint16_t time) {
  pf.combo_pwm(output, pwm);
  pf.single_pwm(output, pwm);
}


// Updated display function that shows both Status and Transmitted Speed
void updateDisplay(String status, String speedText, String details = "") {
  if (status != lastDisplayedStatus || speedText != lastDisplayedSpeed) {
    display.clearBuffer();
    display.setTextColor(EPD_BLACK);
    
    // Header
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.println("TRAIN STATUS:");
    
    // Current Status
    display.setTextSize(2);
    display.setCursor(10, 35);
    display.println(status);
    
    // Transmitted Speed
    display.setTextSize(2);
    display.setCursor(10, 60);
    display.print("SPD: ");
    display.println(speedText);
    
    // Details
    if (details != "") {
      display.setTextSize(1);
      display.setCursor(10, 90);
      display.println(details);
    }
    
    // Timestamp
    display.setTextSize(1);
    display.setCursor(10, 110);
    display.println("Time: " + String(millis() / 1000) + "s");
    
    display.display();
    lastDisplayedStatus = status;
    lastDisplayedSpeed = speedText;
    
    Serial.println("Display Updated - Status: " + status + " | Speed: " + speedText);
  }
}

#if !DEBUG_MODE
void mqttConnect() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(mqttClient)) {
      Serial.println("Connected to MQTT");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("MQTT connection failed, state: ");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

void mqttLoop() {
  if (!client.connected()) {
    mqttConnect();
  }
  client.loop();
}

void performActionBasedOnPayload(byte *payload) {
  Serial.println("MQTT Payload detected");

  if ((char)payload[0] == '0') {
    Serial.println("HALT");
    currentStatus = "STOPPED";
    updateDisplay(currentStatus, "BRAKE", "Emergency brake activated");
    step(RED, PWM_BRK, 0);
  } 
  else if ((char)payload[0] == '1') {
    Serial.println("ADVANCE");
    currentStatus = "RUNNING";
    updateDisplay(currentStatus, "Speed 3", "MQTT Command Executed");
    step(RED, PWM_FWD3, 0);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  performActionBasedOnPayload(payload);
}
#endif


// Debug function to step through FWD 1-7, STOP, REV 1-7, STOP
void debugIRTransmission() {
  unsigned long currentMillis = millis();
  
  // Non-blocking timer check
  if (currentMillis - lastDebugTime >= debugInterval) {
    lastDebugTime = currentMillis;

    static uint8_t debugStep = 0;
    String speedStr = "";
    uint8_t selectedPWM = PWM_BRK;

    // Cycle sequence: 
    // Steps 0 to 6: FWD 1..7
    // Step 7: STOP
    // Steps 8 to 14: REV 1..7
    // Step 15: STOP
    switch (debugStep) {
      case 0:  speedStr = "FWD Speed 1"; selectedPWM = PWM_FWD1; currentStatus = "TEST: FWD"; break;
      case 1:  speedStr = "FWD Speed 2"; selectedPWM = PWM_FWD2; currentStatus = "TEST: FWD"; break;
      case 2:  speedStr = "FWD Speed 3"; selectedPWM = PWM_FWD3; currentStatus = "TEST: FWD"; break;
      case 3:  speedStr = "FWD Speed 4"; selectedPWM = PWM_FWD4; currentStatus = "TEST: FWD"; break;
      case 4:  speedStr = "FWD Speed 5"; selectedPWM = PWM_FWD5; currentStatus = "TEST: FWD"; break;
      case 5:  speedStr = "FWD Speed 6"; selectedPWM = PWM_FWD6; currentStatus = "TEST: FWD"; break;
      case 6:  speedStr = "FWD Speed 7"; selectedPWM = PWM_FWD7; currentStatus = "TEST: FWD MAX"; break;
      
      case 7:  speedStr = "BRAKE / STOP"; selectedPWM = PWM_BRK;  currentStatus = "TEST: STOPPED"; break;

      case 8:  speedStr = "REV Speed 1"; selectedPWM = PWM_REV1; currentStatus = "TEST: REV"; break;
      case 9:  speedStr = "REV Speed 2"; selectedPWM = PWM_REV2; currentStatus = "TEST: REV"; break;
      case 10: speedStr = "REV Speed 3"; selectedPWM = PWM_REV3; currentStatus = "TEST: REV"; break;
      case 11: speedStr = "REV Speed 4"; selectedPWM = PWM_REV4; currentStatus = "TEST: REV"; break;
      case 12: speedStr = "REV Speed 5"; selectedPWM = PWM_REV5; currentStatus = "TEST: REV"; break;
      case 13: speedStr = "REV Speed 6"; selectedPWM = PWM_REV6; currentStatus = "TEST: REV"; break;
      case 14: speedStr = "REV Speed 7"; selectedPWM = PWM_REV7; currentStatus = "TEST: REV MAX"; break;
      
      case 15: speedStr = "BRAKE / STOP"; selectedPWM = PWM_BRK;  currentStatus = "TEST: STOPPED"; break;
    }

    Serial.println("--- [IR DEBUG TX] ---");
    Serial.println("Transmitting Speed: " + speedStr);

    // 1. Update Display with the speed being transmitted
    updateDisplay(currentStatus, speedStr, "DEBUG MODE (16-Step)");

    // 2. Transmit IR Command via PowerFunctions
    step(RED, selectedPWM, 0);

    // Advance to next test step (0 to 15)
    debugStep = (debugStep + 1) % 16;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  delay(1000);

  // Pin mode for IR Emitter
  pinMode(IR_TRANS_IN, OUTPUT);
  digitalWrite(IR_TRANS_IN, LOW);

  // Initialize display settings
  display.begin(THINKINK_MONO);
  display.clearBuffer();
  display.setTextWrap(false);

#if DEBUG_MODE
  Serial.println("=== RUNNING IN DEBUG MODE (No WiFi / No MQTT) ===");
  updateDisplay("DEBUG MODE", "STOPPED", "IR Output Only");
#else
  updateDisplay("STARTING", "N/A", "Connecting to WiFi...");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());  

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  mqttConnect();
  
  updateDisplay("IDLE", "STOPPED", "Waiting for commands...");
#endif
}

void loop() {
#if DEBUG_MODE
  debugIRTransmission();    // Exclusively run IR test and display updates
#else
  mqttLoop();               // Process MQTT network messages in production mode
#endif
}