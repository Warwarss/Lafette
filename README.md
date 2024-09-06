ESP32-S3-N8R8 Program for generating PPM signals in a range between 1000-2000 and also establishing an asynchronous webserver to receive user input through the Esp32' own Wi-Fi Acces Point.

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

    
