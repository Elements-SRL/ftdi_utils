#include "ftdicalibrationeeprom.h"
#include "ftd2xxwrapper.h"

FtdiCalibrationEeprom::FtdiCalibrationEeprom() {
    config.ClockRate = 3000000; /*! 3MHz */
    config.LatencyTimer = 1; /*! 1ms */
    config.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS5 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
    config.Pin = 0x00002323;
}

bool FtdiCalibrationEeprom::openConnection(std::string serial) {
    if (connectionOpened) {
        return true;
    }

    FT_STATUS status;
    Init_libMPSSE();

    handle = Ftd2xxWrapper::SPIW_OpenChannelBySerial(serial);
    if (handle == nullptr) {
        return false;
    }

    status = SPI_InitChannel(handle, &config);
    if (status != FT_OK) {
        return false;
    }

    connectionOpened = true;
    if (!this->enableFPGA(false)) {
        connectionOpened = false;
        return false;
    }

    return true;
}

bool FtdiCalibrationEeprom::closeConnection() {
    if (!connectionOpened) {
        return true;
    }

    if (!this->enableFPGA(true)) {
        return false;
    }

    FT_STATUS status = SPI_CloseChannel(handle);
    if (status != FT_OK) {
        return false;
    }

    Cleanup_libMPSSE();

    connectionOpened = false;
    return true;
}

bool FtdiCalibrationEeprom::enableFPGA(bool flag) {
    if (!connectionOpened) {
        return false;
    }

    FT_STATUS status;
    if (flag) {
        status = FT_WriteGPIO(handle, CEE_XCBUS_DIR, CEE_SPI_PROG_DISABLE | CEE_FPGA_RESET_DISABLE);
    }
    else {
        status = FT_WriteGPIO(handle, CEE_XCBUS_DIR, CEE_SPI_PROG_ENABLE | CEE_FPGA_RESET_ENABLE);
    }

    if (status != FT_OK) {
        return false;
    }
    return true;
}

bool FtdiCalibrationEeprom::enableWrite() {
    if (!connectionOpened) {
        return false;
    }

    FT_STATUS status;

    DWORD bytesWritten[1] = {0};
    int bytesToWrite = 0;
    writeBuffer[bytesToWrite++] = CEE_WRITE_ENABLE_CMD;
    status = SPI_Write(handle, writeBuffer, bytesToWrite, bytesWritten, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    if (status != FT_OK) {
        return false;
    }

    return true;
}

bool FtdiCalibrationEeprom::writeBytes(unsigned char * values, unsigned int addr, unsigned int size) {
    if (!connectionOpened) {
        return false;
    }

    if (addr+size > CEE_EEPROM_SIZE) {
        return false;
    }

    FT_STATUS status;

    /*! Write is disabled by successful write */
    if (!this->enableWrite()) {
        return false;
    }

    DWORD bytesWritten[1] = {0};
    int bytesToWrite = 0;
    writeBuffer[bytesToWrite++] = CEE_PROGRAM_CMD;
    writeBuffer[bytesToWrite++] = (unsigned char)((addr & 0xFF00) >> 8);
    writeBuffer[bytesToWrite++] = (unsigned char)(addr & 0x00FF);
    status = SPI_Write(handle, writeBuffer, bytesToWrite, bytesWritten, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
    if (status != FT_OK) {
        return false;
    }

    status = SPI_Write(handle, values, size, bytesWritten, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    if (status != FT_OK) {
        return false;
    }
    return this->pollWriteDone();
}

bool FtdiCalibrationEeprom::readBytes(unsigned char * values, unsigned int addr, unsigned int size) {
    if (!connectionOpened) {
        return false;
    }

    if (addr >= CEE_EEPROM_SIZE) {
        return false;
    }

    FT_STATUS status;
    DWORD bytesWritten[1] = {0};
    DWORD bytesRead[1] = {0};

    int bytesToWrite = 0;
    writeBuffer[bytesToWrite++] = CEE_READ_CMD;
    writeBuffer[bytesToWrite++] = (unsigned char)((addr >> 8) & 0x00FF);
    writeBuffer[bytesToWrite++] = (unsigned char)(addr & 0x00FF);
    status = SPI_Write(handle, writeBuffer, bytesToWrite, bytesWritten, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
    if (status != FT_OK) {
        return false;
    }

    status = SPI_Read(handle, values, size, bytesRead, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    if (status != FT_OK) {
        return false;
    }

    return true;
}

bool FtdiCalibrationEeprom::readByte(unsigned char * value, unsigned int addr, bool start, bool end) {
    if (!connectionOpened) {
        return false;
    }

    if (addr >= CEE_EEPROM_SIZE) {
        return false;
    }

    FT_STATUS status;
    DWORD bytesWritten[1] = {0};
    DWORD bytesRead[1] = {0};

    if (start) {
        int bytesToWrite = 0;
        writeBuffer[bytesToWrite++] = CEE_READ_CMD;
        writeBuffer[bytesToWrite++] = (unsigned char)((addr & 0xFF00) >> 8);
        writeBuffer[bytesToWrite++] = (unsigned char)(addr & 0x00FF);
        status = SPI_Write(handle, writeBuffer, bytesToWrite, bytesWritten, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
        if (status != FT_OK) {
            return false;
        }
    }

    if (end) {
        status = SPI_Read(handle, value, 1, bytesRead, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    }
    else {
        status = SPI_Read(handle, value, 1, bytesRead, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);
    }
    if (status != FT_OK) {
        return false;
    }
    return true;
}

bool FtdiCalibrationEeprom::pollWriteDone() {
    if (!connectionOpened) {
        return false;
    }

    unsigned char eepromStatus = CEE_WRITE_BUSY;
    while (eepromStatus & CEE_WRITE_BUSY) { /*! \todo FCON magari mettere un timeout */
        this->getStatus(eepromStatus);
    }
    return true;
}

bool FtdiCalibrationEeprom::getStatus(unsigned char &eepromStatus) {
    if (!connectionOpened) {
        return false;
    }

    int bytesToWrite = 0;
    int bytesToRead = 0;
    DWORD bytesWritten[1] = {0};
    DWORD bytesRead[1] = {0};

    writeBuffer[bytesToWrite++] = CEE_GET_STATUS_CMD;
    FT_STATUS status = SPI_Write(handle, writeBuffer, bytesToWrite, bytesWritten, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
    if (status != FT_OK) {
        return false;
    }

    bytesToRead = 1;
    status = SPI_Read(handle, readBuffer, bytesToRead, bytesRead, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    if (status != FT_OK) {
        return false;
    }

    eepromStatus = readBuffer[0];
    return true;
}

unsigned int FtdiCalibrationEeprom::getMemorySize() {
    return CEE_EEPROM_SIZE;
}
