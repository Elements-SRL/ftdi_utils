#ifndef USBDEVICEPRESENCE_H
#define USBDEVICEPRESENCE_H

#include <cstdint>
#include <vector>

enum class UsbPresence {
    Present,    /*!< At least one USB device with one of the vendor ids is plugged in */
    Absent,     /*!< No USB device with those vendor ids */
    Unknown     /*!< The check could not be performed */
};

/*! \brief Looks for a plugged-in USB device with one of the given vendor ids.
 *
 *  It asks the operating system directly (Windows: Configuration Manager, Linux: sysfs), so it
 *  works without any FTDI library and without any driver installed for the device.
 *
 *  \note Internal to ftdi_utils.
 */
UsbPresence findUsbDeviceByVendorId(const std::vector<uint16_t> &vendorIds);

#endif // USBDEVICEPRESENCE_H
