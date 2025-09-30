# CM4-WRT-A
This repository contains the source code for the software and firmware necessary to utilize the custom features of the CM4-WRT-A: A Raspberry Pi CM4 Router Baseboard with NVME support. 
Currently sold on Tindie: https://www.tindie.com/products/mytechcatalog/rpi-cm4-router-baseboard-with-nvme/

The repository is organized as follows:

| Directory | Description |
| --------- | ----------- |
| [CM4](CM4)  | Source code for picod: A service for communicating with the Raspberry Pi Pico            |
| [pico](pico)| Firmware for the Raspberry Pi Pico microcontroller                                                |
| [case](case)| STL files for a 3D printed case for the CM4-WRT-A                                                 |

# Documentation
* [Overview of the CM4-WRT-A](https://www.mytechcatalog.com/CM4-WRT-A-Raspberry-Pi-CM4-Router-Baseboard-With-NVME-Support)
* [How to install OpenWRT on the CM4-WRT-A router](https://www.mytechcatalog.com/how-to-install-openwrt-on-the-cm4-wrt-a-router)
* [How to monitor the CM4-WRT-A board temperatures and fan speed (RPM) in InfluxDB](https://www.mytechcatalog.com/how-to-monitor-the-cm4-wrt-a-board-temperatures-and-fan-speed-in-influxdb)

[![Complimentary Video](https://img.youtube.com/vi/28v230jvPTA/maxresdefault.jpg)](https://www.youtube.com/watch?v=28v230jvPTA)

## Install Prerequisites
The build scripts for the CM4-WRT-A rely on Docker. If you would like to build OpenWrt or the Raspberry Pi Pico firmware for the CM4-WRT-A, install Docker with these commands on the terminal in Ubuntu or Debian: See https://docs.docker.com/desktop/install/linux-install/ for further details.
```shell
sudo apt update
sudo apt install -y docker.io
sudo usermod -aG docker $USER
```
## Building OpenWrt (with picod included) ##
It will be possible to update this OpenWrt build by adding kmods and packages from the official repository using <b>opkg</b>, or <b>LuCI</b> software interface.

1. Clone this repository: 
    ```shell
    git clone https://github.com/MyTechCatalog/cm4-wrt-a.git
    ```
2. Run the OpenWrt build script: 
    ```shell
    cd cm4-wrt-a && ./build-openwrt.sh
    ```
3. The OpenWrt images will be located in this folder: ```cm4-wrt-a/bin/targets/bcm27xx/bcm2711/```

4. Upon first boot up, install the following kmods: `kmod-i2c-core kmod-i2c-bcm2835 kmod-nvme`
    ```code
    root@OpenWrt:~# opkg update
    root@OpenWrt:~# opkg install kmod-i2c-core kmod-i2c-bcm2835 kmod-nvme
    ```

The build script will use the latest tag/version from the OpenWrt repo. If you need to build a specific OpenWrt release version in step 2 above, specify the tag name after the script, for example: 
`./build-openwrt.sh v23.05.6`
### Verifying that picod is running on OpenWrt
After running the following command: `ubus call picod status`, you should see output similar to:
```code
root@OpenWrt:~# ubus call picod status
{
    "fan_pwm_pct": {
        "System_Fan_J17": 50,
        "CM4_FAN_J18": 50
    },
    "watchdog": {
        "is_enabled": "false",
        "timeout_sec": 10,
        "max_retries": 0
    },
    "temperature_c": {
        "PCIe_Switch": "36.2",
        "M.2_Socket_M_J5": "34.5",
        "M.2_Socket_E_J3": "34.8",
        "M.2_Socket_M_J2": "33.0",
        "RPi_Pico": "37.4",
        "Under_CM4_SOC": "36"
    },
    "tachometer_rpm": {
        "System_Fan_J17": 3420,
        "CM4_FAN_J18": 0
    }
}
```
Otherwise, if you get an error such as the one below:
```code
root@OpenWrt:~# ubus call picod status
Command failed: Not found
```
It means that the name of the CM4's serial port (device path) connected to the RPi Pico, is different 
from the default value in the <b>picod</b> configuration file `/etc/picod.conf`.

Edit line 29 in the config file `/etc/picod.conf` by replacing `/dev/ttyAMA3` with the <b>second</b> device path obtained from the output of the following command:
```code
root@OpenWrt:~# ls /dev/ttyA*
/dev/ttyAMA0  /dev/ttyAMA1
```
For example, based on the above output, line 29 of `/etc/picod.conf` should be edited to read:
```code
pico_serial_device_path="/dev/ttyAMA1"
```
Start the <b>picod</b> service, and then verify that it is running as [described above](#verifying-that-picod-is-running-on-openwrt):
```code
root@OpenWrt:~# /etc/init.d/picod start
```
## Building the picod service for Raspbian or Debian variants ##
### Cross-compiling with Docker
Assuming you have Docker installed as described in the [prerequisites](#install-prerequisites) above, run the commands below:

```shell
cd CM4
make docker-cross
./build-picod.sh
```
The debian package <b>picod_1.0_arm64.deb</b> will be located in `CM4/build` folder.
Copy it to the Raspberry Pi and install it with this command:
```shell
dpkg -i picod_1.0_arm64.deb
```
### Starting the picod service (on Raspbian)
```shell
sudo systemctl daemon-reload
sudo systemctl enable picod.service
sudo systemctl start picod.service
```
You can then access the `picod` web interface at <b>http://localhost:8086/</b>. This location is not accessible from outside the RPi CM4 itself. As such, an SSH tunnel can be created with the following command to forward port 8086 to <b>localhost</b> on your computer:
```shell
ssh -L 8086:localhost:8086 username@cm4_ip_address
```
### Update config.txt
Copy to `CM4/config_rpios.txt` to `/boot/firmware/config.txt` in order to configure/enable the custom interfaces on the board.

## Compiling the Raspberry Pi Pico firmware ##
1. Clone this repository if you haven't already done so from above: 
    ```shell
    git clone https://github.com/MyTechCatalog/cm4-wrt-a.git
    ```
2. Run the Pico build script: 
    ```shell
    cd cm4-wrt-a && ./build-pico-firmware.sh
    ```
3. The firmware will be located here : ```cm4-wrt-a/pico/build/cm4-wrt-a.uf2```

## Contact information
Email: cm4-wrt-a@mytechcatalog.com


