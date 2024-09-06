/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef TMP103_I2C_H
#define TMP103_I2C_H
#include "syshead.h"
#include <string>
#include "i2c.h"
#include "Utils.hpp"

// Register map. See "Table 4. Pointer Addresses" in the
// TMP103 datasheet
#define TEMPERATURE_REGISTER_RO     0x00
#define CONFIGURATION_REGISTER_RW   0x01
#define T_LOW_REGISTER_RW           0x02
#define T_HIGH_REGISTER_RW          0x03


namespace picod {

// Contains address information for all models
// of the TMP103 temperature sensor
typedef struct TMP103_I2C_INFO {
    /// @brief Model name
    const char * modelName;

    /// @brief Seven (7) bit i2c bus address
    uint8_t address;

    /// @brief True if a device was detected at given address.
    /// False otherwise.
    bool present;
} TMP103_I2C_INFO;

typedef struct TMP103_REG {
    /// @brief Register name
    const char * name;

    /// @brief Pointer Address
    uint8_t address;
} TMP103_REG;


class TMP103_I2C
{
public:
    static TMP103_I2C &instance();   
    ~TMP103_I2C();;
    TMP103_I2C(TMP103_I2C const&)      = delete;
    void operator=(TMP103_I2C const&)  = delete;

    /// @brief Get the number of detected TMP103 devices 
    /// on the i2c bus
    /// @return Number of devices detected
    size_t getNumDevices();

    /// @brief Prints out register values for all detected
    /// TMP103 devices on the bus
    void dumpDeviceRegisters();

    /// @brief Returns the temperature(s) measured by the TMP103 
    /// @param deviceIndex Zero based index of device (zone)
    /// @return Temperature in degrees C
    int getTemperature(size_t deviceIndex = 0);
private:
    /// @brief The TMP103 is available in eight versions
    // Ref: TMP103 datasheet SBOS545B
    const size_t MAX_NUM_DEVICES_;

    /// @brief Number of registers in the TMP103 register map
    const size_t NUM_REGISTERS_;
    
    /// @brief Device path of i2c bus, such as /dev/i2c-1 
    std::string devicePath_;
    
    /// @brief Device connection info
    I2CDevice device_;

    /// @brief File descriptor
    int fd_;
    
    /// @brief True if at least one(1) device was successfully detected
    bool initDone_;

    /// @brief Number of devices detected
    size_t numDevices_;

    TMP103_I2C(std::string devicePath);
    
    /// @brief Decodes the temperature value from the TMP103
    /// @param dataIn Raw temperature value from the device
    /// @return Temperature in degrees C
    inline int decodeTemperature(uint8_t dataIn);
};

} //@END namespace picod

#endif//TMP103_I2C_H