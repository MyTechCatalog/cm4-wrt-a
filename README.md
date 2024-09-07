# CM4-WRT-A
This repository contains the source code for the software and firmware necessary to utilize the custom features of the CM4-WRT-A: A Raspberry Pi CM4 Router Baseboard with NVME support. 
Currently sold on Tindie: https://www.tindie.com/products/mytechcatalog/rpi-cm4-router-baseboard-with-nvme/

The repository is organized as follows:

| Directory   | Description                                                                                       |
| ------------|---------------------------------------------------------------------------------------------------|
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
1. Clone this repository: 
    ```shell
    git clone https://github.com/MyTechCatalog/cm4-wrt-a.git
    ```
2. Run the OpenWrt build script: 
    ```shell
    cd cm4-wrt-a && ./build-openwrt.sh v23.05.4
    ```
3. The OpenWrt images will be located in this folder: ```cm4-wrt-a/bin/targets/bcm27xx/bcm2711/```

You can omit the OpenWrt release version in step 2 above (<b>v23.05.4</b>), and the script will use the latest tag from the OpenWrt repo.

## Building the picod service for Raspbian or Debian variants ##
### Cross-compiling with Docker
Assuming you have Docker installed as described in the [prerequisites](https://github.com/MyTechCatalog/cm4-wrt-a#install-prerequisites) above, run the commands below:

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


