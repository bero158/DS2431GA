#include "sony_ribbon.h"

TagDate::TagDate() : _year(0), _month(0), _day(0), _valid(false) {}

TagDate::TagDate(const byte *raw8) {
  parse(raw8);
}

uint16_t TagDate::year() const {
  return _year;
}

byte TagDate::month() const {
  return _month;
}

byte TagDate::day() const {
  return _day;
}

bool TagDate::isValid() const {
  return _valid;
}

String TagDate::toString() const {
  if (!_valid) {
    return String("(none)");
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%04u-%02u-%02u",
           (unsigned)_year, (unsigned)_month, (unsigned)_day);
  return String(buf);
}

/* UTC midnight; 0 if invalid. Useful for expiry comparisons. */
time_t TagDate::toUnixTime() const {
  if (!_valid) {
    return 0;
  }

  struct tm t;
  memset(&t, 0, sizeof(t));
  t.tm_year = _year - 1900;
  t.tm_mon = _month - 1;
  t.tm_mday = _day;
  return mktime(&t);
}

void TagDate::parse(const byte *r) {
  _year = _month = _day = 0;
  _valid = false;

  for (byte i = 0; i < 8; ++i) {
    if (r[i] < '0' || r[i] > '9') {
      return;
    }
  }

  _year = (uint16_t)((r[0] - '0') * 1000 + (r[1] - '0') * 100 +
                     (r[2] - '0') * 10 + (r[3] - '0'));
  _month = (byte)((r[4] - '0') * 10 + (r[5] - '0'));
  _day = (byte)((r[6] - '0') * 10 + (r[7] - '0'));
  _valid = (_month >= 1 && _month <= 12 && _day >= 1 && _day <= 31);
}

SonyRibbonTag::SonyRibbonTag() {
  clear();
}

SonyRibbonTag::SonyRibbonTag(const byte *data128) {
  setData(data128);
}

void SonyRibbonTag::clear() {
  memset(_data, 0x20, DS2431_MEM_SIZE);
}

void SonyRibbonTag::setData(const byte *data128) {
  memcpy(_data, data128, DS2431_MEM_SIZE);
}

const byte *SonyRibbonTag::data() const {
  return _data;
}

byte *SonyRibbonTag::data() {
  return _data;
}

String SonyRibbonTag::partNumber() const {
  return text(OFF_PART_NUMBER, 8);
}

String SonyRibbonTag::subCode() const {
  return text(OFF_SUB_CODE, 3);
}

String SonyRibbonTag::serial() const {
  return text(OFF_SERIAL, 15);
}

String SonyRibbonTag::batchCode() const {
  return text(OFF_BATCH, 6);
}

String SonyRibbonTag::reserved() const {
  return text(OFF_RESERVED, 4);
}

bool SonyRibbonTag::isKnownMedia() const {
  return partNumber() == "UPC-R204";
}

TagDate SonyRibbonTag::specDate() const {
  return TagDate(_data + OFF_SPEC_DATE);
}

TagDate SonyRibbonTag::lotDate() const {
  return TagDate(_data + OFF_LOT_DATE);
}

uint16_t SonyRibbonTag::capacity() const {
  return be16(OFF_CAPACITY);
}

uint16_t SonyRibbonTag::remaining() const {
  return be16(OFF_REMAINING);
}

uint16_t SonyRibbonTag::printsUsed() const {
  uint16_t c = capacity();
  uint16_t r = remaining();
  return (r > c) ? 0 : (uint16_t)(c - r);
}

byte SonyRibbonTag::percentRemaining() const {
  uint16_t c = capacity();
  return c ? (byte)((uint32_t)remaining() * 100 / c) : 0;
}

bool SonyRibbonTag::isDepleted() const {
  return remaining() == 0;
}

void SonyRibbonTag::setRemaining(uint16_t prints) {
  if (prints > capacity()) {
    prints = capacity();
  }
  setBe16(OFF_REMAINING, prints);
}

void SonyRibbonTag::resetToFull() {
  setRemaining(capacity());
}

uint16_t SonyRibbonTag::code1() const {
  return be16(OFF_CODE1);
}

uint16_t SonyRibbonTag::code2() const {
  return be16(OFF_CODE2);
}

uint16_t SonyRibbonTag::code3() const {
  return be16(OFF_CODE3);
}

byte SonyRibbonTag::rowOf(uint16_t offset) {
  return (byte)(offset / DS2431_SCRATCH_SIZE);
}

void SonyRibbonTag::copyRow(byte row, byte *out8) const {
  memcpy(out8, _data + (uint16_t)row * DS2431_SCRATCH_SIZE, DS2431_SCRATCH_SIZE);
}

String SonyRibbonTag::toString() const {
  String s;
  s += "Part      : " + partNumber() + "\n";
  s += "Sub code  : " + subCode() + "\n";
  s += "Spec date : " + specDate().toString() + "\n";
  s += "Lot date  : " + lotDate().toString() + "\n";
  s += "Serial    : " + serial() + "\n";
  s += "Batch     : " + batchCode() + "\n";
  s += "Capacity  : " + String(capacity()) + "\n";
  s += "Remaining : " + String(remaining()) +
       " (" + String(percentRemaining()) + "%)\n";
  s += "Used      : " + String(printsUsed()) + "\n";
  return s;
}

uint16_t SonyRibbonTag::be16(uint16_t off) const {
  return (uint16_t)((_data[off] << 8) | _data[off + 1]);
}

void SonyRibbonTag::setBe16(uint16_t off, uint16_t v) {
  _data[off] = (byte)(v >> 8);
  _data[off + 1] = (byte)(v & 0xFF);
}

String SonyRibbonTag::text(uint16_t off, byte len) const {
  char buf[17];
  if (len > 16) {
    len = 16;
  }
  memcpy(buf, _data + off, len);
  buf[len] = '\0';
  for (int16_t i = len - 1; i >= 0 && (buf[i] == ' ' || buf[i] == '\0'); --i) {
    buf[i] = '\0';
  }
  return String(buf);
}

bool SonyRibbonTag::remainingW(uint16_t quantity, uint8_t *firstRow, uint8_t *rowCount) {
  if (quantity > capacity()) {
    return false;
  }
  setBe16(OFF_REMAINING, quantity);
  return rowsOf(OFF_REMAINING, 2, firstRow, rowCount);
}

bool SonyRibbonTag::resetToFullW(uint8_t *firstRow, uint8_t *rowCount) {
  return remainingW(capacity(), firstRow, rowCount);
}

bool SonyRibbonTag::capacityW(uint16_t quantity, uint8_t *firstRow, uint8_t *rowCount) {
  setBe16(OFF_CAPACITY, quantity);
  return rowsOf(OFF_CAPACITY, 2, firstRow, rowCount);
}

bool SonyRibbonTag::partNumberW(const String &text, uint8_t *firstRow, uint8_t *rowCount) {
  setText(OFF_PART_NUMBER, 8, text);
  return rowsOf(OFF_PART_NUMBER, 8, firstRow, rowCount);
}

bool SonyRibbonTag::subCodeW(const String &text, uint8_t *firstRow, uint8_t *rowCount) {
  setText(OFF_SUB_CODE, 3, text);
  return rowsOf(OFF_SUB_CODE, 3, firstRow, rowCount);
}

bool SonyRibbonTag::serialW(const String &text, uint8_t *firstRow, uint8_t *rowCount) {
  setText(OFF_SERIAL, 15, text);
  return rowsOf(OFF_SERIAL, 15, firstRow, rowCount);
}

bool SonyRibbonTag::batchCodeW(const String &text, uint8_t *firstRow, uint8_t *rowCount) {
  setText(OFF_BATCH, 6, text);
  return rowsOf(OFF_BATCH, 6, firstRow, rowCount);
}

bool SonyRibbonTag::lotDateW(const String &yyyymmdd, uint8_t *firstRow, uint8_t *rowCount) {
  if (yyyymmdd.length() != 8) {
    return false;
  }
  setText(OFF_LOT_DATE, 8, yyyymmdd);
  return rowsOf(OFF_LOT_DATE, 8, firstRow, rowCount);
}

bool SonyRibbonTag::specDateW(const String &yyyymmdd, uint8_t *firstRow, uint8_t *rowCount) {
  if (yyyymmdd.length() != 8) {
    return false;
  }
  setText(OFF_SPEC_DATE, 8, yyyymmdd);
  return rowsOf(OFF_SPEC_DATE, 8, firstRow, rowCount);
}

bool SonyRibbonTag::setDataW(const uint8_t *data128, uint8_t *firstRow, uint8_t *rowCount) {
  memcpy(_data, data128, DS2431_MEM_SIZE);
  if (firstRow) {
    *firstRow = 0;
  }
  if (rowCount) {
    *rowCount = DS2431_MEM_SIZE / DS2431_SCRATCH_SIZE;
  }
  return true;
}

void SonyRibbonTag::rowData(uint8_t row, uint8_t *out8) const {
  memcpy(out8, _data + (uint16_t)row * DS2431_SCRATCH_SIZE, DS2431_SCRATCH_SIZE);
}

bool SonyRibbonTag::rowsOf(uint16_t off, uint8_t length,
                           uint8_t *firstRow, uint8_t *rowCount) const {
  if (length == 0 || off + length > DS2431_MEM_SIZE) {
    return false;
  }
  uint8_t first = (uint8_t)(off / DS2431_SCRATCH_SIZE);
  uint8_t last = (uint8_t)((off + length - 1) / DS2431_SCRATCH_SIZE);
  if (firstRow) {
    *firstRow = first;
  }
  if (rowCount) {
    *rowCount = (uint8_t)(last - first + 1);
  }
  return true;
}

void SonyRibbonTag::setText(uint16_t off, uint8_t width, const String &text) {
  for (uint8_t i = 0; i < width; ++i) {
    _data[off + i] = (i < text.length()) ? (uint8_t)text[i] : 0x20;
  }
}
