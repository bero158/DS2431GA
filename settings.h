#pragma once

//#define _DEBUG //log debug info
#define ENABLE_OTA //OTA allowed

#define ONE_WIRE_MAX_DEVICE 1 //max count. of Dallas devices

#define SPIFFS_FOLDER "/spiffs"
#define BACKUP_FOLDER "/spiffs/backup"

//Pins definition
#if defined(ARDUINO_ARCH_ESP32)
    const int signalPin = 4; // 1-Wire data GPIO on ESP32
#else
    const int signalPin = A4;
#endif
const int ledChipDetectPin = 13;  // Use a non-strapping GPIO for stable boot/upload
const int ledWritePin = 23; // LED to indicate writing to EEPROM
const int button1Pin = 19; // Button to trigger reading EEPROM
const int button2Pin = 18; // Button to trigger writing EEPROM


