// Arduino board configuration: works on Arduino Uno and ESP32.
// For ESP32, connect DS2431 to 3V3/GND and use a 4.7k pull-up on the data pin.
#include <Arduino.h>
#include <OneWire.h>
#include <jled.h>
#include "fs_esp_idf.h"
#include "ds2431.h"
#include "sony_ribbon.h"
#include "pushbutton.h"
#include "settings.h"
#include "version.h"
#include "system.h"
#include "cpuTimer.h"
#include <mbedtls/base64.h>
PushButton button1;
PushButton button2;
Filesystem fs;
boolean chipDetected = false;

OneWire ds(signalPin); // 1-wire on the configured data pin
byte addr[8];          // Contains the eeprom unique ID
DS2431 ds2431(ds, addr);

JLed chipDetectedLED = JLed(ledChipDetectPin);
JLed writeLED = JLed(ledWritePin);
JLed watchdogLED = JLed(LED_BUILTIN).Blink(2000, 200).Forever();
cpuTimer detectChipTimer = cpuTimer(1000, true, true); //when user clicked up/down btns show preset temp for X seconds


String printAddress(byte *address, String delimitor = ":" )
{
  int i;
  String out = "";
  for (i = 0; i < 8; i++)
  {
    char buffer[3];
    sprintf(buffer, "%02X", address[i]);
    out += buffer + (i < 7 ? delimitor : "");
  }
  return out;
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println("DS2431 EEPROM Reader/Writer ver. " + String(VERSION) + " build: " + String(BUILD_DATE));
  Serial.println("EEPROM size: " + String(DS2431_MEM_SIZE) + " bytes");
  Serial.println("1-Wire data pin: " + String(signalPin));

  button1.interval(30);
  button1.attach(button1Pin, INPUT);
  button2.interval(30);
  button2.attach(button2Pin, INPUT);

  
  if (!fs.mount(false))
  {
    Serial.println("Failed to mount SPIFFS filesystem");
    format();
    if (!fs.mount(false))
    {
      Serial.println("Failed to mount SPIFFS filesystem after formatting");
      watchdogLED.Blink(200, 200).Forever();
    }
  }
  else
  {
    Serial.println("SPIFFS filesystem mounted");
  }
  fs.unmount();
  menu();
}

void dir(String root = SPIFFS_FOLDER) {
   if (fs.mount()) {
    Serial.println("Directory listing of SPIFFS filesystem:");
    for (const auto &entry : Dir::ls(root))
    {
      Serial.println(entry);
    }
    fs.unmount();
  }
}
void rmFile(String file) {
   if (fs.mount()) {
    Serial.println("Removing file: " + file);
    Dir::rm(file);
    fs.unmount();
  }
}
void menu(void)
{
  Serial.println("Menu:");
  Serial.println("  EEPROM:");
  Serial.println("    e - erase memory");
  Serial.println("    h - read memory as hexadecimal");
  Serial.println("    b - backup memory to SPIFFS file");
  Serial.println("    w - recovery memory from SPIFFS file");
  Serial.println("    t - read backup as hexadecimal");
  Serial.println("    u - read backup as base64 (for download)");
  Serial.println("    s - store base64 data as SPIFFS backup file");
  Serial.println("    x - read control bytes");
  Serial.println("    c - write chars");
  Serial.println(" Sony specific:");
  Serial.println("    a - write remaining prints");
  Serial.println("    o - parse Sony ribbon tag");
  Serial.println("    p - parse backup of Sony ribbon tag");
  Serial.println(" System:");
  Serial.println("    r - reboot");
  Serial.println("    d - Directory listing of SPIFFS filesystem");
  Serial.println("    f - format SPIFFS filesystem");
  Serial.println("    k - delete SPIFFS file");
  Serial.println("    m - print menu");
  Serial.println("Press a key to select an option");
}



void loop(void)
{
  watchdogLED.Update();
  chipDetectedLED.Update();
  writeLED.Update();
  serialEvent();
  switch (button1.update())
  {

  case PushButton::press::shortpress:
  Serial.println("Button 1 short press");
    break;
  case PushButton::press::longpress:
    Serial.println("Button 1 long press");
    Serial.println("Rebooting ...");
    systemFNC::restart("User requested reboot");
    break;
  }
switch (button2.update())
  {
  case PushButton::press::shortpress:
  Serial.println("Button 2 short press");
    writeLED.On();
    break;
  case PushButton::press::longpress:
    Serial.println("Button 2 long press");
    break;
  }
  if (detectChipTimer.isOverTime()) 
  {
    if (ds2431.SearchAddress(addr))
    {
      chipDetectedLED.On();
      if ( !chipDetected ) {
        Serial.println("Detected DS2431 EEPROM with address: " + printAddress(addr));
        menu();
      }
      chipDetected = true;
    } else {
      chipDetectedLED.Off();
      chipDetected = false;
    }
  }
}


void writeString(const String &text)
{
  Serial.println("Writing chars to EEPROM ");
  byte textBytes[DS2431_MEM_SIZE];
  ds2431.stringToByteArray(text, textBytes);

  byte dat[13];
  for (int row = 0; row < 16; row++)
  { // EEPROM has 16 rows of 8 bytes
    for (int B = 0; B < 8; B++)
    { // load eight bytes at a tim6; row++){
      dat[B] = textBytes[row * DS2431_SCRATCH_SIZE + B];
    }
    Serial.print(".");
    ds2431.WriteRow(row, dat); // write the bytes out to EEPROM
    delay(100);
  }
  Serial.println("Done");
}


boolean SearchAddress(byte *address)
{ // Search for address and confirm it
  if (!ds2431.SearchAddress(address))
  {
    Serial.println("No device found.");
    return false;
  }

  Serial.println("ADDR= " + printAddress(address));

  if (OneWire::crc8(address, 7) != address[7])
  {
    Serial.println("CRC is not valid, address is corrupted");
    return false;
  }

  switch (address[0])
  {
  case 0xAD:
    Serial.println("Device is a Sony DS custom EEPROM.");
    break;
  case DS2431_FAMILY_CODE:
    Serial.println("Device is a DS2431 EEPROM.");
    break;
  default:
    Serial.println("Device is not a DS2431 EEPROM.");
    return false;
  }

  Serial.println();
  return true;
}





void printHex(const byte *data)
{
  for (int row = 0; row < DS2431_MEM_SIZE/DS2431_SCRATCH_SIZE; row++)
  { // EEPROM has 16 rows of 8 bytes
    Serial.print(row);
    if (row < 10)
    {
      Serial.print("  ");
    }
    else
    {
      Serial.print(" ");
    }
    for (int B = 0; B < DS2431_SCRATCH_SIZE; B++)
    { // load eight bytes at a time
      Serial.printf("%02X ", data[row * 8 + B]);
    }
    Serial.println();
  }
}

void ReadAllMemHex()
{
  Serial.println("Reading from EEPROM as HEX");
  ds2431.ReadStart();
  byte data[DS2431_MEM_SIZE];

  for (int i = 0; i < DS2431_MEM_SIZE; i++)

  { // EEPROM has 16 rows of 8 bytes
    data[i] = ds.read();
  }
  
  printHex(data);
}

void ReadProtectionAndUserBytes()
{
  Serial.println("Reading Protection Control and User Bytes (0x80-0x86):");
  byte data[7];
  ds2431.ReadProtectionAndUserBytes(data);
  Serial.printf("0x80 Page 0 protection control = 0x%02X\n", data[0]);
  Serial.printf("0x81 Page 1 protection control = 0x%02X\n", data[1]);
  Serial.printf("0x82 Page 2 protection control = 0x%02X\n", data[2]);
  Serial.printf("0x83 Page 3 protection control = 0x%02X\n", data[3]);
  Serial.printf("0x84 Factory byte (read-only) = 0x%02X\n", data[4]);
  Serial.printf("0x85 User byte 1 = 0x%02X\n", data[5]);
  Serial.printf("0x86 User byte 2 = 0x%02X\n", data[6]);
}

boolean readBackupFile(const String &filename, byte *buffer)
{
  boolean ret = false;
  String trimmed = filename;
  trimmed.trim();
  if (fs.mount()) {
    File file(trimmed, "rb");
    if (file) {
      size_t bytesRead = file.read(buffer, DS2431_MEM_SIZE);
      if (bytesRead == DS2431_MEM_SIZE) {
        ret = true;
      } else {
        Serial.println("Failed to read enough data from file: " + filename);
      }
      file.close();
    } else {
      Serial.println("Failed to open file for reading: " + filename);
    }
    fs.unmount();
  } else {
    Serial.println("Failed to mount SPIFFS filesystem");
  }
  return ret;
}

void recovery(const String &filename)
{
  if (SearchAddress(addr)) {
    byte buffer[DS2431_MEM_SIZE];
    if (readBackupFile(filename, buffer)) {
          ds2431.Write(buffer);
          Serial.println("Memory recovered from " + filename);
        }
  }
}
void readFileHex(const String &filename)
{
  byte buffer[DS2431_MEM_SIZE];
  if (readBackupFile(filename, buffer)) {
    printHex(buffer);
  }
}
void writeFileBase64(const String &filename, const String &base64Data)
{
  byte buffer[DS2431_MEM_SIZE];
  size_t olen;
  int ret = mbedtls_base64_decode(buffer, sizeof(buffer), &olen, (const unsigned char *)base64Data.c_str(), base64Data.length());
  if (ret != 0) {
    Serial.println("Failed to decode base64 data");
    return;
  }
  if (olen != DS2431_MEM_SIZE) {
    Serial.println("Decoded data size does not match expected EEPROM size");
    return;
  }

  if (fs.mount()) {
    File file(filename, "wb");
    if (file) {
      file.write(buffer, DS2431_MEM_SIZE);
      file.close();
      Serial.println("Base64 data written to " + filename);
    } else {
      Serial.println("Failed to open file for writing: " + filename);
    }
    fs.unmount();
  } else {
    Serial.println("Failed to mount SPIFFS filesystem");
  }
}
void readFileBase64(const String &filename)
{
  byte buffer[DS2431_MEM_SIZE];
  if (readBackupFile(filename, buffer)) {
    char encoded[DS2431_MEM_SIZE * 2]; // Base64 encoded size
    size_t olen;
    mbedtls_base64_encode((unsigned char *)encoded, sizeof(encoded), &olen, buffer, DS2431_MEM_SIZE);
    Serial.println(encoded);
  }
}

void erase()
{
  Serial.print("Writing zeros EEPROM ");
  ds2431.Erase();
  Serial.println("Done");
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
byte command = 0;
String inputBuffer; // Buffer to hold incoming data

void clearCommand()
{
  command = 0;
  inputBuffer = ""; // Clear the buffer
}
void serialEvent()
{
  byte inChar;
  String filename  = "";
  while (Serial.available())
  {
    // get the new byte:
    inChar = (byte)Serial.read();
    if (command)
    {
      if ((inChar == '\n' || inChar == '\r') && inputBuffer.length() == 0) 
      {
        // ignore empty lines
        continue;
      }
      if (inChar == 27 || inChar == 0x3)
      { // ESC key, Ctrl+C
        Serial.println("Command cancelled.");
        clearCommand();
        return;
      }
      if (inChar == '\n' || inChar == '\r')
      {
        // end of input, process the command
        switch (command)
        {
        case 'a':
        
          int copies;
          copies = inputBuffer.toInt();
          if (copies <= 0 || copies > 700)
          {
            Serial.println("Invalid number of copies. Please enter a positive integer less than or equal to 700.");
            return;
          }
          Serial.println("Writing " + String(copies) + " remaining prints to EEPROM");
          writeRemaining(copies);
          break;

        case 'c':
          if (SearchAddress(addr))
          {
            writeLED.On();
            writeString(inputBuffer);
            writeLED.Off();
          }
          break;
        case 'w':
          filename = String(BACKUP_FOLDER) + "/" + inputBuffer;
          recovery(filename);
          break;
        case 't':
          filename = String(BACKUP_FOLDER) + "/" + inputBuffer;
          readFileHex(filename);
          break;  
        case 'p':
          filename = String(BACKUP_FOLDER) + "/" + inputBuffer;
          byte buffer[DS2431_MEM_SIZE];
          readBackupFile(filename, buffer);
          parseBuffer(buffer);
          break;  
        case 'u':
          filename = String(BACKUP_FOLDER) + "/" + inputBuffer;
          readFileBase64(filename);
          break;
         case 'k':
          filename = String(SPIFFS_FOLDER) + "/" + inputBuffer;
          rmFile(filename);
          break;
        case 's':
          //separate filename and base64 data by space
          int spaceIndex = inputBuffer.indexOf(' ');
          if (spaceIndex != -1) {
            filename = String(BACKUP_FOLDER) + "/" + inputBuffer.substring(0, spaceIndex);
            String base64Data = inputBuffer.substring(spaceIndex + 1);
            writeFileBase64(filename, base64Data);
          } else {
            Serial.println("Invalid input. Please provide filename followed by base64 data separated by space.");
          }
          break;
        }
        clearCommand();
      }
      else
      {
        inputBuffer += (char)inChar; // append to input buffer
      }
    }
    else
    {
      switch (inChar)
      {
      case 'e':
        if (SearchAddress(addr))
        {
          writeLED.On();
          erase();
          writeLED.Off();
        }
        break;

      
      case 'h':
        if (SearchAddress(addr))
        {
          ReadAllMemHex();
        }
        break;

      case 'x':
        if (SearchAddress(addr))
        {
          ReadProtectionAndUserBytes();
        }
        break;

      case 'b':
        if (SearchAddress(addr))
        {
          uint8_t buffer[DS2431_MEM_SIZE];
          ds2431.ReadMemory(buffer, DS2431_MEM_SIZE);
          SonyRibbonTag tag(buffer);
          if (fs.mount()) {
            String filename = String(BACKUP_FOLDER) + "/" + printAddress(addr, "") + "_" + String(tag.remaining()) + ".bin";
            File file(filename, "wb");
            if (file)
            {
              file.write(buffer, DS2431_MEM_SIZE);
              file.close();
              Serial.println("Backup saved to " + filename);
            }
            else
            {
              Serial.println("Failed to open file for writing: " + filename);
            }
            fs.unmount();
          } else {
            Serial.println("Failed to mount SPIFFS filesystem");
          }
        }
        break;

      case 's':
        command = 's';
        Serial.println("Enter filename followed by base64 data separated by space to write SPIFFS backup file:");
        break;
      case 'd':
        dir();
        break;

      case 'c':
        command = 'c';
        Serial.println("Enter string to write to EEPROM (max 128 chars):");
        break;
      case 'w':
        command = 'w';
        Serial.println("Enter filename to recover memory from SPIFFS file:");
        dir(BACKUP_FOLDER);
        break;
      case 't':
        command = 't';
        Serial.println("Enter filename to read backup as hexadecimal from SPIFFS file:");
        dir(BACKUP_FOLDER);
        break;
      case 'p':
        command = 'p';
        Serial.println("Enter filename to parse backup of Sony ribbon tag from SPIFFS file:");
        dir(BACKUP_FOLDER);
        break;
      case 'u':
        command = 'u';
        Serial.println("Enter filename to read backup as base64 from SPIFFS file:");
        dir(BACKUP_FOLDER);
        break;
      case 'k':
        command = 'k';
        Serial.println("Enter filename to delete from SPIFFS:");
        dir();
        break;

        case 'm':
        menu();
        break;

      case 'o':
        parse();
        break;
      case 'r':
        systemFNC::restart("User requested reboot");
        break;

      case 'a':
        command = 'a';
        Serial.println("How many copies?");
        break;
      case 'f':
      format();
      break;
      }
    }
  }
}

void format() {
        Serial.println("Formatting SPIFFS filesystem...");
      if (fs.format()) {
        Dir::md(SPIFFS_FOLDER);
        Dir::md(BACKUP_FOLDER);
        Serial.println("SPIFFS filesystem formatted successfully.");
      } else {
        Serial.println("Failed to format SPIFFS filesystem.");
      }
}
void parseBuffer(byte *buffer)
{
    SonyRibbonTag tag(buffer);
    Serial.println(tag.toString());
}

void parse()
{
  if (SearchAddress(addr))
  {
    uint8_t buffer[DS2431_MEM_SIZE];
    ds2431.ReadMemory(buffer, DS2431_MEM_SIZE);
    parseBuffer(buffer);
  }
}

void writeRemaining(uint16_t remaining)
{
  if (SearchAddress(addr))
  {
    writeLED.On();
    uint8_t buffer[DS2431_MEM_SIZE];
    ds2431.ReadMemory(buffer, DS2431_MEM_SIZE);
    SonyRibbonTag tag(buffer);
    uint8_t row, count;
    tag.remainingW(remaining, &row, &count);
    for (uint8_t r = row; r < row + count; r++)
    {
      uint8_t rowData[8];
      tag.rowData(r, rowData);
      ds2431.WriteRow(r, rowData);
    }
    writeLED.Off();
  }
}
