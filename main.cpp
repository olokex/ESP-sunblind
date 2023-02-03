#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Stepper.h>

const char* ssid = "";
const char* password = "";

constexpr int HTTP_OK = 200;
constexpr int HTTP_NOT_SUPPORTED = 505;

constexpr int JSON_BUFFER_SIZE = 500;
const char* HOSTNAME = "ESP test";
constexpr int PORT = 43332;

int stepsPerRevolution = 2048;
int steppingSpeed = 3;
int step = 1;
int stepping = 0;
int maxOpen = 1000;
int maxClose = -1000;
int stepsCounter = 0;
bool handlingRequest = false;

constexpr int IN1 = 4;
constexpr int IN2 = 0;
constexpr int IN3 = 2;
constexpr int IN4 = 15;

Stepper myStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);

WebServer server(PORT);

void reset() {
  stepping = 0;
  handlingRequest = false;
  
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
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

  stepsPerRevolution = jsonDocument["stepsPerRevolution"];
  steppingSpeed = jsonDocument["steppingSpeed"];
  maxOpen = jsonDocument["maxOpen"];
  maxClose = jsonDocument["maxClose"];
  step = jsonDocument["step"];
  stepsCounter = jsonDocument["stepsCounter"];

  Stepper newStepper(stepsPerRevolution, IN1, IN3, IN2, IN4);
  myStepper = newStepper;
  myStepper.setSpeed(steppingSpeed);

  server.send(HTTP_OK, "application/json", "{}");
}

void getInfo() {
  String jsonResponse;
  StaticJsonDocument<JSON_BUFFER_SIZE> jsonDocument;

  jsonDocument["stepsPerRevolution"] = stepsPerRevolution;
  jsonDocument["steppingSpeed"] = steppingSpeed;
  jsonDocument["maxOpen"] = maxOpen;
  jsonDocument["maxClose"] = maxClose;
  jsonDocument["step"] = step;
  jsonDocument["stepsCounter"] = stepsCounter;

  serializeJson(jsonDocument, jsonResponse);
  server.send(HTTP_OK, "application/json", jsonResponse);
}

void restoreDefaultConfig() {
  stepsPerRevolution = 2048;
  steppingSpeed = 3;
  step = 1;
  stepping = 0;
  maxOpen = 1000;
  maxClose = -1000;
  handlingRequest = false;
  stepsCounter = 0;

  getInfo();
}

void setup_routing() { 
  server.on("/left", HTTP_POST, [](){handlePost(step);});    
  server.on("/right", HTTP_POST, [](){handlePost(-step);});  
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

  pinMode(18, INPUT);
  pinMode(19, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  myStepper.setSpeed(steppingSpeed);

  setup_routing();
  Serial.begin(9600);
}

//====================================================
//         Loop
//====================================================
void loop() {
  if (digitalRead(18)) {
    stepping = -step;
  } else if (digitalRead(19)) {
    stepping = step;
  } else {
    if (!handlingRequest) {
      reset();
    }
  }

  if (stepping > 0 && stepsCounter + stepping <= maxOpen ) {
    myStepper.step(stepping);
    stepsCounter += stepping;
  }
  
  if (stepping < 0 && stepsCounter - stepping >= maxClose ) {
    myStepper.step(stepping);
    stepsCounter += stepping;
  }

  server.handleClient();
}