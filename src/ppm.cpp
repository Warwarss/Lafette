#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"
#include <VescUart.h>

// PPM signal parameters
const int ppmPinR = 18; // GPIO pin for Right PPM signal
const int ppmPinL = 17; // GPIO pin for Left PPM signal
const int ppmChannels = 3; // Number of PPM channels
int ppmValues[ppmChannels] = {1500, 1500, 1500}; // PPM values for each channel
// motor starts turning at 1620 ppm
// pulselength start at 1.0000 ms
// pulselength end at 2.0000 ms
// pulselength center at 1.500 ms
void generatePPM(int Microseconds);
SemaphoreHandle_t ppmMutex;
SemaphoreHandle_t timerSemaphore;
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// Wifi credentials
const char* ssid = "EmirhanÜZÜM!!!";
const char* password = "kannmanmachen";
// For the webserver
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void IRAM_ATTR onTimer() {
  // Trigger the task to generate the PPM signal
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
}

void generatePPMTask(void *parameter) {
  int counter = 0;
  while (1) {
    int highTime;

    // Read the shared variable with mutex protection
    xSemaphoreTake(ppmMutex, portMAX_DELAY);
    highTime = ppmValues[0];
    xSemaphoreGive(ppmMutex);

    // Wait until the timer interrupt triggers
    if (xSemaphoreTake(timerSemaphore, portMAX_DELAY) == pdTRUE) {
    generatePPM(highTime);
    xSemaphoreGive(timerSemaphore);
    }
    if (counter == 5){
      Serial.println("I am running generatePPMTask");
      counter = 0;
    }
    // MCU breaks down without this delay, dunno why
    counter ++;
    vTaskDelay(1);
  }
}
void GradualValueChangeTask(void *parameter) {
  int *params = (int *)parameter;
  int newValue = params[0];
  int oldValue = params[1];
  int channel = params[2];
  unsigned long previousTime = millis();
  
  int tickCounter = 0;
  int stepSize = 10;
  int transientValue = oldValue;
  int change = newValue - oldValue;
  int stepCount = abs(change) / stepSize;
  int diff = abs(change) % stepSize;

  Serial.printf("newValue: %d, oldValue: %d, channel: %d, tickCounter: %d, stepSize: %d, transientValue: %d, change: %d, stepCount: %d, diff: %d\n",
            newValue, oldValue, channel, tickCounter, stepSize, transientValue, change, stepCount, diff); 

  if (change < 0){
    stepSize = -stepSize;
    diff = -diff;
  } 

  for (int i = 0; i < stepCount; i++){
    transientValue = transientValue + stepSize;

    Serial.printf("newValue: %d, oldValue: %d, channel: %d, tickCounter: %d, stepSize: %d, transientValue: %d, change: %d, stepCount: %d, diff: %d\n",
              newValue, oldValue, channel, tickCounter, stepSize, transientValue, change, stepCount, diff);     
    
    xSemaphoreTake(ppmMutex, portMAX_DELAY);
    ppmValues[channel] = transientValue;
    xSemaphoreGive(ppmMutex);
    // Wait for 5 timer interrupts)
    while (tickCounter < 50){
      unsigned long currentTime = millis();
      if (xSemaphoreTake(timerSemaphore, portMAX_DELAY) == pdTRUE) {
      tickCounter++;
      xSemaphoreGive(timerSemaphore);
      unsigned long elapsedTime = currentTime - previousTime;
      Serial.printf("Time difference: %lu ms\n", elapsedTime); // Print the time difference
      previousTime = currentTime;
      }
    }
    tickCounter = 0;
    vTaskDelay(5);  
  }
  Serial.println(transientValue);
  transientValue = transientValue + diff;
  xSemaphoreTake(ppmMutex, portMAX_DELAY);
  ppmValues[channel] = transientValue;
  xSemaphoreGive(ppmMutex);

  vTaskDelete(NULL);
}
void generatePPM(int Microseconds = 1500) {
  // generatePPM signal for 2 channels
  // Ensure that the PPM signal is within the valid range
  if (Microseconds < 1000) {
    Microseconds = 1000;
  } else if (Microseconds > 2000) {
    Microseconds = 2000;
  }

  ledcWrite(0, 255);
  ledcWrite(1, 255);
  delayMicroseconds(Microseconds);
  ledcWrite(0, 0);
  ledcWrite(1, 0);
  delayMicroseconds(20000 - Microseconds);
}

void initFS() {
  // Initialize LittleFS, a file system for microcontrollers
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else{
   Serial.println("LittleFS mounted successfully");
  }
}
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len){
  //Communication between the Website User interface and the ESP32
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  String message = "";
    for (size_t i = 0; i < len; i++) {
      //Data coming from the websocket is written byte-wise into the message variable 
      message += (char) data[i];
    }
  Serial.print("Message: "); Serial.println(message);
  int channel = message.substring(0, 1).toInt();
  Serial.print("Channel: "); Serial.println(channel);
  int value = message.substring(2).toInt();
  Serial.print("Value: "); Serial.println(value);
  Serial.print("Updated PPM channel "); Serial.println(value);
  if (channel >= 0 && channel < ppmChannels) {
  int oldValue = ppmValues[0];
  if (abs(value - oldValue) > 30){
    int *params = new int[3]{value, oldValue, channel};
    Serial.println("Gradual value change task created");
    xTaskCreatePinnedToCore(GradualValueChangeTask, "GradualValueChangeTask", 2048, params, 2, NULL, 1);
  }
  else{  
    xSemaphoreTake(ppmMutex, portMAX_DELAY);
    ppmValues[0] = value;
    xSemaphoreGive(ppmMutex);
  }
  }
}   

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  //Websocket event handler function
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}



void setup() {
  // Disabling watchdog timers. JUST FOR DEBUGGING
  disableCore0WDT();
  disableCore1WDT();
  disableLoopWDT();
  
  // Initialize serial communication
  Serial.begin(9600);
  while(!Serial);
  delay(1000);
  Serial.println("Initialization complete!");
  // Initialize LittleFS (File system for microcontrollers)
  initFS();
  // Initialize Wifi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println(IP);
  // Web Server Root URL, Serve the HTML file
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.serveStatic("/", LittleFS, "/");
  // Inıtialize WebSocket
  ws.onEvent(onEvent);
  server.addHandler(&ws);
  // Start server
  server.begin();
  // Initialize timer
  timerSemaphore = xSemaphoreCreateBinary();
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 20000, true);
  timerAlarmEnable(timer);
  // Initialize LEDC for PPM signal
  ledcSetup(0, 40000, 1); // Channel 0, 40 kHz frequency, 1-bit resolution
  ledcAttachPin(ppmPinR, 0); // Attach GPIO pin to channel 0
  ledcSetup(1, 40000, 1); // Channel 1, 40kHz frequency, 1-bit resolution
  ledcAttachPin(ppmPinL, 1); // Attach GPIO pin to channel 1
  ppmMutex = xSemaphoreCreateMutex();
  delay(1000);
  // Create tasks
  xTaskCreatePinnedToCore(
    generatePPMTask,   // Task function
    "PPM Task",        // Name of the task
    10000,             // Stack size
    NULL,              // Task input parameter
    1,                 // Priority of the task
    NULL,              // Task handle
    0                  // Core where the task should run
  );
}

void loop() {
  if (Serial.available()){
    String massage = Serial.readStringUntil('\n');
    Serial.println(massage);
    int command = massage.toInt();
    if (command >= 1000 && command <= 2000){
      xSemaphoreTake(ppmMutex, portMAX_DELAY);
      ppmValues[0] = command;
      xSemaphoreGive(ppmMutex);
      Serial.print("Updated PPM high time to: ");
      Serial.println(command);
    } 
    else {
      Serial.println("Invalid command");
    }
  }
}
