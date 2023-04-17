/**
 * Copyright (c) 2023 MyTechCatalog LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include "TMP103_I2C.hpp"
#include "Utils.hpp"

namespace picod {

TMP103_I2C_INFO tmp_i2c_info[] = 
{
    {"TMP103A", 0x70, false },
    {"TMP103B", 0x71, false },
    {"TMP103C", 0x72, false },
    {"TMP103D", 0x73, false },
    {"TMP103E", 0x74, false },
    {"TMP103F", 0x75, false },
    {"TMP103G", 0x76, false },
    {"TMP103H", 0x77, false }
};

TMP103_REG tmp103_reg_map[] = 
{
    {"TEMPERATURE", TEMPERATURE_REGISTER_RO},
    {"CONFIGURATION", CONFIGURATION_REGISTER_RW},
    {"T_LOW", T_LOW_REGISTER_RW},
    {"T_HIGH", T_HIGH_REGISTER_RW}
};

TMP103_I2C::TMP103_I2C(std::string devicePath)
:MAX_NUM_DEVICES_(ARRAY_SIZE(tmp_i2c_info)),
NUM_REGISTERS_(ARRAY_SIZE(tmp103_reg_map)),
devicePath_(devicePath), fd_(0), initDone_(false),
numDevices_(0)
 {    
    // Open i2c bus
    if ((fd_ = i2c_open(devicePath_.c_str())) == -1) {

        print_err("Failed to open i2c bus: %s, %s\n",
            devicePath_.c_str(), strerror(errno));
        return;
    }
    
    device_.bus = fd_;
    device_.addr = tmp_i2c_info[1].address;
    device_.tenbit = 0;
    device_.delay = 10;
    device_.flags = 0;
    device_.page_bytes = 8;
    device_.iaddr_bytes = 0;
    
    // Detect the number of devices (zones) present
    numDevices_ = 0;

    for (size_t i = 0; i < MAX_NUM_DEVICES_; i++) {
        device_.addr = tmp_i2c_info[i].address;

        uint8_t data = 0;
        ssize_t ret = i2c_ioctl_write(&device_, 0x0, &data, 1);
        if ( (ret == -1) || ((size_t)ret != 1) ) {
            //print_err("Write failed on i2c bus: %s, %s\n",
            //    devicePath_.c_str(), strerror(errno));
            continue;
        }
        
        ret = i2c_ioctl_read(&device_, 0x0, &data, 1);
        if ( (ret == -1) || ((size_t)ret != 1) ) {
            //print_err("Read failed on i2c bus: %s, %s\n",
            //    devicePath_.c_str(), strerror(errno));
            continue;
        }

        tmp_i2c_info[i].present = true;
        numDevices_++;
    }//@END for (size_t i = 0; i < MAX_NUM_DEVICES; i++)

    if (numDevices_ == 0) {
        print_err("Error, failed to detect any TMP103 devices on i2c bus: %s\n",
           devicePath_.c_str());
    }   

    initDone_ = (numDevices_ > 0);
}

TMP103_I2C::~TMP103_I2C() {
    i2c_close(fd_);
}

size_t TMP103_I2C::getNumDevices(){
    return numDevices_;
}

void TMP103_I2C::dumpDeviceRegisters(){
    
    for (size_t i = 0; i < MAX_NUM_DEVICES_; i++) {
        device_.addr = tmp_i2c_info[i].address;
       
       if (!tmp_i2c_info[i].present) {
            continue;
       }
       
       printf("%s, I2C address: 0x%02X\n", devicePath_.c_str(), tmp_i2c_info[i].address);
       
       for (size_t i = 0; i < NUM_REGISTERS_; i++) {
            uint8_t data = tmp103_reg_map[i].address;
            ssize_t ret = i2c_ioctl_write(&device_, 0x0, &data, 1);
            if ( (ret == -1) || ((size_t)ret != 1) ) {
                print_err("Write failed on i2c bus: %s, %s\n",
                    devicePath_.c_str(), strerror(errno));
                continue;
            }
            
            data = 0;
            ret = i2c_ioctl_read(&device_, 0x0, &data, 1);
            if ( (ret == -1) || ((size_t)ret != 1) ) {
                print_err("Read failed on i2c bus: %s, %s\n",
                    devicePath_.c_str(), strerror(errno));
                continue;
            }

            if (i != 1) {
                printf("\t%s, PTR: 0x%02X, Data: 0x%02X (%d °C)\n", 
                    tmp103_reg_map[i].name, tmp103_reg_map[i].address, 
                        data, decodeTemperature(data));
            } else {
                printf("\t%s, PTR: 0x%02X, Data: 0x%02X\n", 
                    tmp103_reg_map[i].name, tmp103_reg_map[i].address, data);
            }            
       }        
    }//@END for (size_t i = 0; i < MAX_NUM_DEVICES; i++)
}//#END dumpDeviceRegisters()

inline int TMP103_I2C::decodeTemperature(uint8_t dataIn){
    // See datasheet: Twos complement is not performed 
    // on positive numbers.
    int retVal = dataIn;
    
    if(!!(dataIn & 0x80)){
        // This is a negative value, sign extend it.
        retVal = 0xFFFFFF00 | dataIn;     
    }

    return retVal;
}

int TMP103_I2C::getTemperature(size_t deviceIndex){
    int retVal = -273;
    // Sanitize the input
    size_t idx = deviceIndex % MAX_NUM_DEVICES_;
    
    if (!tmp_i2c_info[idx].present) {
        return retVal;
    }

    device_.addr = tmp_i2c_info[idx].address;

    uint8_t data = tmp103_reg_map[TEMPERATURE_REGISTER_RO].address;
    ssize_t ret = i2c_ioctl_write(&device_, 0x0, &data, 1);
    if ( (ret == -1) || ((size_t)ret != 1) ) {
        print_err("Write failed on i2c bus: %s, %s\n",
            devicePath_.c_str(), strerror(errno));
    }

    data = 0;
    ret = i2c_ioctl_read(&device_, 0x0, &data, 1);
    if ( (ret == -1) || ((size_t)ret != 1) ) {
        print_err("Read failed on i2c bus: %s, %s\n",
            devicePath_.c_str(), strerror(errno));                
    } else {
        retVal = decodeTemperature(data);
    }

    return retVal;
}

TMP103_I2C & TMP103_I2C::instance() {    
    static TMP103_I2C tmp103(appSettings.tmp103_i2c_device_path);
#if 0
    static bool initDone = false;
    if (!initDone) {
        initDone = true;        
        if (tmp103.getNumDevices() > 0) {
            tmp103.dumpDeviceRegisters();
        }        
    }
#endif

    return tmp103;
}
} //@END namespace picod