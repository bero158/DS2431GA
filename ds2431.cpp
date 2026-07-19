#include "ds2431.h"

DS2431::DS2431(OneWire &bus, byte *address)
    : ds_(bus), addr_(address)
{
}

void DS2431::WriteReadScratchPad(byte TA1, byte TA2, byte *data)
{
    ds_.reset();
    ds_.select(addr_);
    ds_.write(DS2431_CMD_WRITE_SCRATCH, 1);
    ds_.write(TA1, 1);
    ds_.write(TA2, 1);
    for (int i = 0; i < DS2431_SCRATCH_SIZE; i++)
    {
        ds_.write(data[i], 1);
    }

    ds_.reset();
    ds_.select(addr_);
    ds_.write(DS2431_CMD_READ_SCRATCH);

    for (int i = 0; i < 13; i++)
    {
        data[i] = ds_.read();
    }
}

void DS2431::CopyScratchPad(byte *data)
{
    ds_.reset();
    ds_.select(addr_);
    ds_.write(DS2431_CMD_COPY_SCRATCH, 1);
    ds_.write(data[0], 1);
    ds_.write(data[1], 1);
    ds_.write(data[2], 1);
    delay(25);
}

void DS2431::WriteRow(byte row, byte *data)
{
    byte buffer[13];
    if (row > 15)
    {
        return;
    }
    memcpy(buffer, data, DS2431_SCRATCH_SIZE);
    WriteReadScratchPad(row * DS2431_SCRATCH_SIZE, 0x00, buffer);
    CopyScratchPad(buffer);
}

/// @brief 
/// @param buffer 
void DS2431::Write(byte *buffer)
{
    for (byte row = 0; row < DS2431_MEM_SIZE / DS2431_SCRATCH_SIZE; row++)
    {
        WriteRow(row, buffer + row * DS2431_SCRATCH_SIZE);
    }
}

void DS2431::ReadStart(uint16_t address)
{
    ds_.reset();
    ds_.select(addr_);
    ds_.write(DS2431_CMD_READ_MEMORY);
    byte TA1, TA2;
    splitAddress(address, &TA1, &TA2);
    ds_.write(TA1);
    ds_.write(TA2);
}

void DS2431::ReadMemory(byte *data, int length)
{
    ReadBuffer(data, length);
}

void DS2431::ReadBuffer(uint8_t *data, int length)
{
    ReadStart(0x0000);
    for (int n = 0; n < length; n++)
    {
        data[n] = ds_.read();
    }
}

void DS2431::ReadRow(byte row, byte *data)
{
    ReadStart((uint16_t)row * DS2431_SCRATCH_SIZE);
    for (int n = 0; n < DS2431_SCRATCH_SIZE; n++)
    {
        data[n] = ds_.read();
    }
}

/* EEPROM address of a row (always 8-byte aligned). */
uint16_t DS2431::RowAddress(byte row)
{
    return (uint16_t)row * DS2431_SCRATCH_SIZE;
}

void DS2431::splitAddress(uint16_t address, byte *ta1, byte *ta2)
{
    if (ta1)
        *ta1 = (byte)(address & 0xFF);
    if (ta2)
        *ta2 = (byte)(address >> 8);
}

boolean DS2431::SearchAddress(byte *address)
{ // Search for address and confirm it
    if (!ds_.search(address))
    {
        // trying again
        if (!ds_.search(address))
        {
            ds_.reset_search();
            delay(250);
            return false;
        }
    }
    return true;
}

void DS2431::ReadProtectionAndUserBytes(byte *data)
{
  ReadStart(0x0080); // Move to address 0x80 (128 in decimal)
  for (int i = 0; i < 7; i++)
  {
    data[i] = ds_.read();
  }
}

void DS2431::Erase()
{
  int i = 0;
  byte dat[13];
  for (byte row = 0; row < 16; row++)
  { // EEPROM has 16 rows of 8 bytes
    for (int B = 0; B < 8; B++)
    { // load eight bytes at a tim6; row++){
      dat[B] = 0;
      i++;
    }
    WriteRow(row, dat); // write the bytes out to EEPROM
    delay(10);
  }
  Serial.println("Done");
}

size_t DS2431::stringToByteArray(const String &src, byte *dst)
{
  size_t length = src.length();
  if (length > DS2431_MEM_SIZE)
  {
    length = DS2431_MEM_SIZE;
  }

  for (size_t i = 0; i < length; i++)
  {
    dst[i] = (byte)src[i];
  }

  for (size_t i = length; i < DS2431_MEM_SIZE; i++)
  {
    dst[i] = 0;
  }

  return length;
}
