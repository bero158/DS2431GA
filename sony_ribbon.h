/*
 * SonyRibbonTag.h
 * ---------------
 * Class wrapper for the 1-Wire EEPROM tag on Sony UP-DR200 / UP-CR20L
 * dye-sub ribbons (DR2046 media, "UPC-R204").
 *
 * Target: ESP32 (Arduino framework).
 *
 * The class keeps the raw 128-byte EEPROM image internally and exposes
 * typed getters/setters. Text fields on the tag are ASCII, space-padded
 * and NOT null-terminated; getters trim the padding and return String.
 * Numeric fields are big-endian uint16 and are byte-swapped on access.
 *
 * Note: String getters allocate on the heap. Cache results rather than
 * calling them repeatedly inside a loop.
 */

#ifndef SONY_RIBBON_TAG_H
#define SONY_RIBBON_TAG_H

#include <Arduino.h>
#include <ctime>
#include "ds2431.h"

/* ================================================================== */
/* Constants                                                           */
/* ================================================================== */


/* Field offsets, derived from tag dumps */
enum TagOffset : uint16_t {
    OFF_PART_NUMBER = 0x00,   /* 8  "UPC-R204"                          */
    OFF_SUB_CODE    = 0x0D,   /* 3  "210"                               */
    OFF_SPEC_DATE   = 0x10,   /* 8  same on all tags -> spec revision   */
    OFF_LOT_DATE    = 0x18,   /* 8  varies per ribbon                   */
    OFF_CODE1       = 0x20,   /* BE16 = 1                               */
    OFF_CODE2       = 0x22,   /* BE16 = 17                              */
    OFF_CODE3       = 0x24,   /* BE16 = 2                               */
    OFF_CAPACITY    = 0x26,   /* BE16 = 700 for 4x6                     */
    OFF_MARKER      = 0x28,   /* 0x01 0x01, constant                    */
    OFF_SERIAL      = 0x2A,   /* 15 alphanumeric                        */
    OFF_BATCH       = 0x40,   /* 6  e.g. "AKT441" / "YHV621"            */
    OFF_RESERVED    = 0x50,   /* 4  "0000"                              */
    OFF_REMAINING   = 0x60    /* BE16 prints remaining (page 3)         */
};



/* ================================================================== */
/* TagDate - tag stores dates as 8 ASCII digits, "YYYYMMDD"            */
/* ================================================================== */

class TagDate {
public:
    TagDate();
    explicit TagDate(const byte *raw8);

    uint16_t year() const;
    byte month() const;
    byte day() const;
    bool isValid() const;

    String toString() const;

    /* UTC midnight; 0 if invalid. Useful for expiry comparisons. */
    time_t toUnixTime() const;

private:
    void parse(const byte *r);

    uint16_t _year;
    byte  _month;
    byte  _day;
    bool     _valid;
};


/* ================================================================== */
/* SonyRibbonTag - the 128-byte data area                              */
/* ================================================================== */

class SonyRibbonTag {
public:
    SonyRibbonTag();
    explicit SonyRibbonTag(const byte *data128);

    void clear();
    void setData(const byte *data128);

    const byte *data() const;
    byte *data();

    /* ---- identity ---- */
    String partNumber() const;
    String subCode() const;
    String serial() const;
    String batchCode() const;
    String reserved() const;

    bool isKnownMedia() const;

    /* ---- dates ---- */
    TagDate specDate() const;
    TagDate lotDate() const;

    /* ---- counters ---- */
    uint16_t capacity() const;
    uint16_t remaining() const;

    uint16_t printsUsed() const;

    byte percentRemaining() const;

    bool isDepleted() const;

    /* ---- modification ---- */
    void setRemaining(uint16_t prints);
    void resetToFull();

    /* ---- misc codes ---- */
    uint16_t code1() const;
    uint16_t code2() const;
    uint16_t code3() const;

    /* ---- helpers ---- */

    /* The 8-byte EEPROM row containing an offset. The counter is in row 12. */
    static byte rowOf(uint16_t offset);

    /* Copy the 8-byte row that must be rewritten to change the counter. */
    void copyRow(byte row, byte *out8) const;

    String toString() const;

    
private:
    uint16_t be16(uint16_t off) const;
    void setBe16(uint16_t off, uint16_t v);

    /* space-padded ASCII field -> trimmed String */
    String text(uint16_t off, byte len) const;

    byte _data[DS2431_MEM_SIZE];

public:
    /* ================================================================
     * Write helpers
     * ----------------------------------------------------------------
     * Each *W() function applies the change to the in-memory image and
     * reports which EEPROM rows became dirty:
     *
     *   firstRow - index of the first affected 8-byte row (0-15)
     *   rowCount - how many consecutive rows are affected
     *
     * The DS2431 writes whole 8-byte aligned rows, so this is exactly
     * what you need to push. Typical use:
     *
     *   uint8_t first, count, buf[8];
     *   if (tag.remainingW(700, &first, &count)) {
     *       for (uint8_t r = first; r < first + count; r++) {
     *           tag.rowData(r, buf);
     *           writeRow(SonyRibbonTag::rowAddress(r), buf);
     *       }
     *   }
     * ================================================================ */
 
    /* Set prints remaining. Returns false (no change) if > capacity. */
    bool remainingW(uint16_t quantity, uint8_t *firstRow, uint8_t *rowCount);
 
    /* Reset the counter to a full pack. */
    bool resetToFullW(uint8_t *firstRow, uint8_t *rowCount);
 
    /* Set total capacity (700 for 4x6, 400 for 5x7, 350 for 6x8). */
    bool capacityW(uint16_t quantity, uint8_t *firstRow, uint8_t *rowCount);
 
    /* Text fields. Longer input is truncated, shorter is space-padded. */
    bool partNumberW(const String &text, uint8_t *firstRow, uint8_t *rowCount);
    bool subCodeW(const String &text, uint8_t *firstRow, uint8_t *rowCount);
    bool serialW(const String &text, uint8_t *firstRow, uint8_t *rowCount);
    bool batchCodeW(const String &text, uint8_t *firstRow, uint8_t *rowCount);
 
    /* Dates: expects exactly 8 ASCII digits, "YYYYMMDD". */
    bool lotDateW(const String &yyyymmdd, uint8_t *firstRow, uint8_t *rowCount);
    bool specDateW(const String &yyyymmdd, uint8_t *firstRow, uint8_t *rowCount);
 
    /* Replace the whole image (e.g. cloning to a blank chip): all 16 rows. */
    bool setDataW(const uint8_t *data128, uint8_t *firstRow, uint8_t *rowCount);
 
    /* Full 8-byte row content - this is what actually gets written. */
    void rowData(uint8_t row, uint8_t *out8) const;
 
   
 private:
    /* Which 8-byte rows does a field at 'off' of 'length' bytes touch? */
    bool rowsOf(uint16_t off, uint8_t length,
                uint8_t *firstRow, uint8_t *rowCount) const;
    /* Write a String into a fixed-width, space-padded ASCII field. */
    void setText(uint16_t off, uint8_t width, const String &text);


};



#endif /* SONY_RIBBON_TAG_H */