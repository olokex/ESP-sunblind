#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Stepper.h>
#include <ArduinoOTA.h>
#include <esp_system.h>
#include <SPIFFS.h>

static const char* CONFIG_FILE = "/config.txt";

struct Configuration_t {
  int32_t stepsPerRevolution = 2048;
  int32_t steppingSpeed = 20;
  int32_t step = 3;
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

void saveConfiguration(const char *filename, const Configuration_t &config) {
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.write((uint8_t *)&config, sizeof(Configuration_t));
  file.close();
}

void loadConfiguration(const char *filename, Configuration_t &config) {
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  file.read((uint8_t *)&config, sizeof(Configuration_t));
  file.close();
}


void reset() {
  stepping = 0;
  handlingRequest = false;
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  
  saveConfiguration(CONFIG_FILE, configuration);
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

  saveConfiguration(CONFIG_FILE, configuration);

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

  saveConfiguration(CONFIG_FILE, configuration);
  getInfo();
}

void handleFullyOpen() {
  StaticJsonDocument<JSON_BUFFER_SIZE> jsonDocument;
  String responseMessage;
  int precalculatedSteps = 0;

  if (configuration.stepsCounter >= 0) {
    precalculatedSteps = -configuration.stepsCounter;
    jsonDocument["message_positive"] = "Moving in positive: " + String(precalculatedSteps);
  }

  if (configuration.stepsCounter <= 0) {
    precalculatedSteps = abs(configuration.stepsCounter);
    jsonDocument["message_negative"] = "Moving in negative: " + String(precalculatedSteps);
  }

  jsonDocument["step_counter"] = configuration.stepsCounter;
  serializeJson(jsonDocument, responseMessage);

  handlingRequest = true;
  myStepper.step(precalculatedSteps);
  configuration.stepsCounter = 0;

  reset();
  server.send(HTTP_OK, "application/json", responseMessage);
}

void handleFullyClose(int direction) {
  stepping = direction;
  handlingRequest = true;

  server.send(HTTP_OK, "application/json", "{}");
}

bool isInRange(int value, int boundary, int delta = 0) {
  // Calculate the lower and upper bounds
  int lowerBound = boundary - delta;
  int upperBound = boundary + delta;

  // Check if the value is within the bounds
  return value >= lowerBound && value <= upperBound;
}

void setupRouting() { 
  server.on("/left", HTTP_POST, [](){handlePost(configuration.step);});    
  server.on("/right", HTTP_POST, [](){handlePost(-configuration.step);});
  server.on("/open", HTTP_POST, handleFullyOpen);
  server.on("/close/left", HTTP_POST, [](){handleFullyClose(configuration.step);});
  server.on("/close/right", HTTP_POST, [](){handleFullyClose(-configuration.step);});
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

  pinMode(LEFT_BUTTON_IN, INPUT);
  pinMode(RIGHT_BUTTON_IN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  setupRouting();
  Serial.begin(9600);
  ArduinoOTA.begin();

  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }

  loadConfiguration(CONFIG_FILE, configuration);
  myStepper.setSpeed(configuration.steppingSpeed);
}

//====================================================
//         Loop
//====================================================
void loop() {
  ArduinoOTA.handle();

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

  if (handlingRequest) {
    if (stepping > 0) {
      if (isInRange(configuration.stepsCounter, configuration.maxOpen, configuration.step)) {
        reset();
      }
    }
    if (stepping < 0) {
      if (isInRange(configuration.stepsCounter, configuration.maxClose, configuration.step)) {
        reset();
      }
    }
  }

  server.handleClient();
}
