#ifndef FTDICALIBRATIONEEPROM_H
#define FTDICALIBRATIONEEPROM_H

#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include "WinTypes.h"
#else
#endif

#include "ftd2xx.h"
#include "libmpsse_spi.h"
#include "stdint.h"
#include "string"

/*! EEPROM constants */
#define CEE_XCBUS_DIR 0x09 /*! Set high output pins: FPGA reset 0x01 + SPI program 0x08 */
#define CEE_SPI_PROG_ENABLE 0x00
#define CEE_SPI_PROG_DISABLE 0x08
#define CEE_FPGA_RESET_ENABLE 0x00
#define CEE_FPGA_RESET_DISABLE 0x01
#define CEE_WRITE_BUSY 0x01
#define CEE_EEPROM_SIZE 2048

/*! Eeprom commands */
#define CEE_PROGRAM_CMD 0x02
#define CEE_READ_CMD 0x03
#define CEE_GET_STATUS_CMD 0x05
#define CEE_WRITE_ENABLE_CMD 0x06

class FtdiCalibrationEeprom {
public:
    FtdiCalibrationEeprom();

    bool openConnection(std::string serial);
    bool closeConnection();
    bool writeBytes(unsigned char * values, unsigned int addr, unsigned int size);
    bool readBytes(unsigned char * values, unsigned int addr, unsigned int size);

    static unsigned int getMemorySize();

private:
    bool enableFPGA(bool flag);
    bool enableWrite();
    bool pollWriteDone();
    bool getStatus(unsigned char &eepromStatus);
    bool readByte(unsigned char * value, unsigned int addr, bool start = true, bool end = true);

    ChannelConfig config;
    FT_HANDLE handle;
    bool connectionOpened = false;

    /*! Buffers */
    unsigned char writeBuffer[256];
    unsigned char readBuffer[256];
};

#endif // FTDICALIBRATIONEEPROM_H
