# CM4-WRT-A
This repository contains the source code for the software and firmware necessary to utilize the custom features of the CM4-WRT-A: A Raspberry Pi CM4 Router Baseboard with NVME support. 
Currently sold on Amazon: https://www.amazon.com/dp/B0C17SX8QG and Tindie: https://www.tindie.com/products/mytechcatalog/rpi-cm4-router-baseboard-with-nvme/

The repository is organized as follows:

| Directory   | Description                                                                                       |
| ------------|---------------------------------------------------------------------------------------------------|
| [CM4](CM4)  | Source code for picod: An OpenWrt service for communicating with the Raspberry Pi Pico            |
| [pico](pico)| Firmware for the Raspberry Pi Pico microcontroller                                                |
| [case](case)| STL files for a 3D printed case for the CM4-WRT-A                                                 |

# Documentation
Overview of the CM4-WRT-A: https://www.mytechcatalog.com/CM4-WRT-A-Raspberry-Pi-CM4-Router-Baseboard-With-NVME-Support

How to install OpenWRT on the CM4-WRT-A router: https://www.mytechcatalog.com/how-to-install-openwrt-on-the-cm4-wrt-a-router

How to monitor the CM4-WRT-A board temperatures and fan speed (RPM) in InfluxDB: https://www.mytechcatalog.com/how-to-monitor-the-cm4-wrt-a-board-temperatures-and-fan-speed-in-influxdb

## Install Prerequisites
The build scripts for the CM4-WRT-A rely on Docker. If you would like to build OpenWrt or the Raspberry Pi Pico firmware for the CM4-WRT-A, install Docker with these commands on the terminal in Ubuntu or Debian: See https://docs.docker.com/desktop/install/linux-install/ for further details.
```shell
sudo apt update
sudo apt install -y docker.io
sudo usermod -aG docker $USER
```
## Building OpenWrt (with picod included) ##
1. Clone this repository: 
    ```shell
    git clone https://github.com/MyTechCatalog/cm4-wrt-a.git
    ```
2. Run the OpenWrt build script: 
    ```shell
    cd cm4-wrt-a && ./build-openwrt.sh v22.03.4
    ```
3. The OpenWrt images will be located in this folder: ```cm4-wrt-a/bin/targets/bcm27xx/bcm2711/```

You can omit the OpenWrt release version in step 2 above (<b>v22.03.4</b>), and the script will use the latest tag from the OpenWrt repo.
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
