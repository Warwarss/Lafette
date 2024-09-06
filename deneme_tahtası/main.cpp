
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "LittleFS.h"


#include <Arduino.h>
#include <VescUart.h>
// HardwareSerial Serial1(1);

// put function declarations here:
int myFunction(int, int);
const char** fetch_data(VescUart& UART);
VescUart UART;

const char** fetch_data(VescUart& UART) {
  static const char* data[4];
  static const char* failure_message = "Failed to get data!";

  if (UART.getVescValues()) {
    static char rpm[16];
    static char inpVoltage[16];
    static char ampHours[16];
    static char tachometerAbs[16];

    snprintf(rpm, sizeof(rpm), "%d", UART.data.rpm);
    snprintf(inpVoltage, sizeof(inpVoltage), "%.2f", UART.data.inpVoltage);
    snprintf(ampHours, sizeof(ampHours), "%.2f", UART.data.ampHours);
    snprintf(tachometerAbs, sizeof(tachometerAbs), "%d", UART.data.tachometerAbs);

    data[0] = rpm;
    data[1] = inpVoltage;
    data[2] = ampHours;
    data[3] = tachometerAbs;
    for (int i = 0; i < 4; i++) {
      Serial.println(data[i]);
    }
  } else {
    data[0] = failure_message;
    data[1] = failure_message;
    data[2] = failure_message;
    data[3] = failure_message;
    Serial.println(failure_message);
  }

  return data;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(1000);
  Serial.println("Initialization complete!");

  Serial1.begin(9600, SERIAL_8N1, 18, 17); // RX, TX pins
  Serial1.println("UART1 initialized!");
  UART.setSerialPort(&Serial1);
}

void loop() {
  // put your main code here, to run repeatedly:
  const char** result = fetch_data(UART);
  Serial.println(result[0]);
  Serial1.println("Data fetched!");
  delay(1000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}