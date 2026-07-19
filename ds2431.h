#ifndef DS2431_H
#define DS2431_H

#include <Arduino.h>
#include <OneWire.h>

/* ---- DS2431 ROM function commands ---- */
#define DS2431_CMD_READ_ROM        0x33  /* single device on bus only */
#define DS2431_CMD_MATCH_ROM       0x55  /* address one specific device */
#define DS2431_CMD_SEARCH_ROM      0xF0  /* enumerate devices on bus   */
#define DS2431_CMD_SKIP_ROM        0xCC  /* address all / single device */
#define DS2431_CMD_RESUME          0xA5  /* re-select last matched device */
#define DS2431_CMD_OD_SKIP_ROM     0x3C  /* overdrive skip ROM */
#define DS2431_CMD_OD_MATCH_ROM    0x69  /* overdrive match ROM */

/* ---- DS2431 memory function commands ---- */
#define DS2431_CMD_WRITE_SCRATCH   0x0F  /* write 8 bytes to scratchpad */
#define DS2431_CMD_READ_SCRATCH    0xAA  /* read back scratchpad + E/S  */
#define DS2431_CMD_COPY_SCRATCH    0x55  /* commit scratchpad to EEPROM */
#define DS2431_CMD_READ_MEMORY     0xF0  /* read memory from an address */

/* ---- Identity ---- */
#define DS2431_FAMILY_CODE         0x2D  /* rom[0] on a genuine DS2431 */

/* ---- Protection control byte values (write to 0x80-0x84) ---- */
#define DS2431_PROT_WRITE_PROTECT  0x55  /* page becomes permanent read-only */
#define DS2431_PROT_EPROM_MODE     0xAA  /* bits may only change 1 -> 0       */
/* any other value = unprotected read/write; both above are irreversible     */

/* ---- Register page addresses ---- */
#define DS2431_ADDR_PROT_PAGE0     0x80
#define DS2431_ADDR_PROT_PAGE1     0x81
#define DS2431_ADDR_PROT_PAGE2     0x82
#define DS2431_ADDR_PROT_PAGE3     0x83
#define DS2431_ADDR_COPY_PROT      0x84
#define DS2431_ADDR_FACTORY_BYTE   0x85

/* ---- Geometry ---- */
#define DS2431_MEM_SIZE            128   /* bytes of user data (0x00-0x7F) */
#define DS2431_PAGE_SIZE          32    /* bytes per page, 4 pages total  */
#define DS2431_SCRATCH_SIZE        8     /* scratchpad size in bytes       */

class DS2431 {
public:
	DS2431(OneWire& bus, byte* address);
	void WriteReadScratchPad(byte TA1, byte TA2, byte* data);
	void CopyScratchPad(byte* data);
	void WriteRow(byte row, byte* buffer);
	void Write(byte* buffer);
	void ReadStart(uint16_t address);
    void ReadStart() { ReadStart(0x0000); }
	void ReadMemory(byte* data, int length);
    void ReadRow(byte row, byte* data);
    void ReadProtectionAndUserBytes(byte *data);
    size_t stringToByteArray(const String &src, byte *dst);
    void Erase();
    boolean SearchAddress(byte* address);
    uint16_t RowAddress(byte row);
private:
	void ReadBuffer(byte* data, int length);
    void splitAddress(uint16_t address, byte *ta1, byte *ta2);
	OneWire& ds_;
	byte* addr_;
};

#endif