#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Stepper.h>
#include <EEPROM.h>

constexpr int EEPROM_SIZE = 64;
constexpr int EEPROM_MEMORY_ADDRESS = 0;

struct Configuration_t {
  int32_t stepsPerRevolution = 2048;
  int32_t steppingSpeed = 3;
  int32_t step = 1;
  int32_t maxOpen = 1000;
  int32_t maxClose = -1000;
  int32_t stepsCounter = 0;
};

Configuration_t configuration;

const char* ssid = "";
const char* password = "";

constexpr int HTTP_OK = 200;
constexpr int HTTP_NOT_SUPPORTED = 505;

const char* HOSTNAME = "ESP Test";
constexpr int JSON_BUFFER_SIZE = 500;
constexpr int PORT = 43332;


int stepping = 0;
bool handlingRequest = false;

constexpr int LEFT_BUTTON_IN = 25;
constexpr int RIGHT_BUTTON_IN = 33;

constexpr int IN1 = 12;
constexpr int IN2 = 14;
constexpr int IN3 = 27;
constexpr int IN4 = 26;

Stepper myStepper(configuration.stepsPerRevolution, IN1, IN3, IN2, IN4);

WebServer server(PORT);

void reset() {
  stepping = 0;
  handlingRequest = false;
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  EEPROM.put(EEPROM_MEMORY_ADDRESS, configuration);
  EEPROM.commit();
}

void handlePost(int direction) {
  StaticJsonDocument<JSON_BUFFER_SIZE> jsonDocument;
  String body = server.arg(0);

  deserializeJson(jsonDocument, body);

  if (jsonDocument["switch"] == "on") {
    stepping = direction;
    handlingRequest = true;
  }
  
  if (jsonDocument["switch"] == "off") {
    reset();
  }

  server.send(HTTP_OK, "application/json", "{}");
}

void setConfiguration() {
  StaticJsonDocument<JSON_BUFFER_SIZE> jsonDocument;
  String jsonResponse = server.arg(0);
  DeserializationError error = deserializeJson(jsonDocument, jsonResponse);

  if (error) {
    server.send(HTTP_NOT_SUPPORTED, "application/json", "{}");
    return;
  }

  configuration.stepsPerRevolution = jsonDocument["stepsPerRevolution"];
  configuration.steppingSpeed = jsonDocument["steppingSpeed"];
  configuration.maxOpen = jsonDocument["maxOpen"];
  configuration.maxClose = jsonDocument["maxClose"];
  configuration.step = jsonDocument["step"];
  configuration.stepsCounter = jsonDocument["stepsCounter"];

  EEPROM.put(EEPROM_MEMORY_ADDRESS, configuration);
  EEPROM.commit();

  Stepper newStepper(configuration.stepsPerRevolution, IN1, IN3, IN2, IN4);
  myStepper = newStepper;
  myStepper.setSpeed(configuration.steppingSpeed);

  server.send(HTTP_OK, "application/json", "{}");
}

void getInfo() {
  String jsonResponse;
  StaticJsonDocument<JSON_BUFFER_SIZE> jsonDocument;

  jsonDocument["stepsPerRevolution"] = configuration.stepsPerRevolution;
  jsonDocument["steppingSpeed"] = configuration.steppingSpeed;
  jsonDocument["maxOpen"] = configuration.maxOpen;
  jsonDocument["maxClose"] = configuration.maxClose;
  jsonDocument["step"] = configuration.step;
  jsonDocument["stepsCounter"] = configuration.stepsCounter;

  serializeJson(jsonDocument, jsonResponse);
  server.send(HTTP_OK, "application/json", jsonResponse);
}

void restoreDefaultConfig() {
  Configuration_t config;
  configuration = config;

  EEPROM.put(EEPROM_MEMORY_ADDRESS, configuration);
  EEPROM.commit();

  getInfo();
}

void setup_routing() { 
  server.on("/left", HTTP_POST, [](){handlePost(configuration.step);});    
  server.on("/right", HTTP_POST, [](){handlePost(-configuration.step);});  
  server.on("/", HTTP_OPTIONS, [](){ server.send(HTTP_OK, "application/json", "{}");} );
  server.on("/configure", HTTP_POST, setConfiguration);
  server.on("/info", HTTP_GET, getInfo);
  server.on("/restoreDefaultConfig", HTTP_GET, restoreDefaultConfig);

  server.begin();    
}

//====================================================
//         Setup
//====================================================
void setup() {
  server.enableCORS(true);

  WiFi.setHostname(HOSTNAME);
  WiFi.begin(ssid, password);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_MEMORY_ADDRESS, configuration);

  pinMode(LEFT_BUTTON_IN, INPUT);
  pinMode(RIGHT_BUTTON_IN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  myStepper.setSpeed(configuration.steppingSpeed);

  setup_routing();
  Serial.begin(9600);
}

//====================================================
//         Loop
//====================================================
void loop() {
  if (digitalRead(LEFT_BUTTON_IN)) {
    stepping = -configuration.step;
  } else if (digitalRead(RIGHT_BUTTON_IN)) {
    stepping = configuration.step;
  } else {
    if (!handlingRequest) {
      reset();
    }
  }

  if (stepping > 0 && configuration.stepsCounter + stepping <= configuration.maxOpen ) {
    myStepper.step(stepping);
    configuration.stepsCounter += stepping;
  }
  
  if (stepping < 0 && configuration.stepsCounter - stepping >= configuration.maxClose ) {
    myStepper.step(stepping);
    configuration.stepsCounter += stepping;
  }

  server.handleClient();
}
