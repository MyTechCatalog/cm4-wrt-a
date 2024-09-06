#ifndef SENSOR_ID_HPP
#define SENSOR_ID_HPP
namespace picod {
    enum SensorId{
        PCIe_Switch,
        Mdot2_Socket_M_J5,
        Mdot2_Socket_E_J3,
        Mdot2_Socket_M_J2,
        RPi_Pico,
        System_FAN_J17,
        CM4_FAN_J18,
        Under_CM4_SOC,
        NUM_SENSOR_IDs
    };
}//@END namespace picod

#endif //SENSOR_ID_HPP