ESP32-S3-N8R8 Program for generating PPM signals in a range between 1000-2000ms and also establishing an asynchronous webserver to receive user input through the Esp32' own Wi-Fi Acces Point.

Before starting to use it:
1. Download Platformio Extension in VSCode
2. Download the Esp32 Component in Platformio
3. Go to C:\Users\<Username>\.platformio\platforms\espressif32\boards
4. Add the file "esp32-s3-devkitc1-n8r8.json" to that folder
5. Its important that you have this file in that boards folder, otherwise you will not be able to build for this specific board because it is not there by default
6. External libraries go into the lib folder
7. You have to create a new folder named "data". This folder contains the files that will be flashed to the Esp32
8. When you change the files in that folder, say for example changing the javascript function or adding a new element by html
9. You have to flash these folders again to the ESP32 via Platformio Project Tasks > Platform > Upload Filesystem Image
10. Otherwise your changes will not be uploaded to the ESP32, even when you upload a new firmware
11. Configure the upload ports in platform.ini file, Example : monitor_port = COM5
12. ESP32-S3 has two "USB" ports named "USB" and "UART" on the board. I suggest using the port named "USB"


I would suggest to read or use this as a referance for dealing with the FREERTOS system
https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/freertos_idf.html
ESP32 has a good referance and documentation availability

Explanation of the code

PPM signal generation function is < void generatePPM(int Microseconds) > 
this function is called periodically by the FREERTOS Task < void generatePPMTask(void *parameter) >.
Function is called periodically with the help of the timer. The timer initialization is not trivial :
https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/timer.html
It is also possible to create another timer when needed, the period of the timer is set in the timerAlarmWrite() function

In the setup function, Important peripherals and API handlers are created ;
Serial communication : 9600 Baud Rate, communication through the USB cable connected to the PC
Little FS : File system
WiFi Acces point : So that wireless comunication is first made possible
  Wifi ID : EmirhanÜZÜM!!!
  Wifi password : kannmanmachen
Websocket : This is needed for the communication between the ESP32 firmware and the ESPAsyncWebServer
Webserver : Hosting the http URL and serving the .html file
Timer : Timer is initialized and an interrupt routine is attached to it
LEDC : This is a hardware peripheral built into the SoC. It is needed to generate the PPM signals, DONT MESS WITH THE SETUP

