# Navio2 RCIO Driver

This directory contains the container environment used to build and install the **Navio2 RCIO kernel driver**.

The driver provides the interface between the Raspberry Pi and the **Navio2 autopilot HAT**, enabling access to:

- PWM outputs
- ADC inputs
- SPI communication with the Navio microcontroller

This repository contains a migrated version of the original driver from:

https://github.com/emlid/rcio-dkms

The original implementation is only compatible with older kernels and 32-bit systems.  
The version provided here builds successfully on **Linux kernel 6.x** with **64-bit Ubuntu**.


## Build and Install the Driver

The driver is built inside a container to ensure that the required kernel headers and build tools are available.

Run the following commands **on the Raspberry Pi**:

```bash
sudo podman build -t rcio-dkms .
```

Start the container to compile and install the module (ensure you replace the kernel version in the volume mounts):

```bash
sudo podman run --privileged \
    -v /lib/modules:/lib/modules \
    -v /etc/modules-load.d:/usr/src/rcio/modules-load.d \
    -v /usr/src/linux-headers-6.8.0-2036-raspi-realtime:/usr/src/linux-headers-6.8.0-2036-raspi-realtime \
    -v /usr/src/linux-raspi-realtime-headers-6.8.0-2036:/usr/src/linux-raspi-realtime-headers-6.8.0-2036 \
    -v /boot/firmware/overlays:/usr/src/rcio/overlays \
    rcio-dkms
```

After installation the module will be registered using **DKMS**, allowing it to rebuild automatically when the kernel is updated.


## Configure Raspberry Pi Interfaces

The Navio2 board requires several hardware interfaces to be enabled.

Edit:

```
/boot/firmware/config.txt
```

Add the following configuration:

```
dtparam=spi=on
dtoverlay=spi0-4cs
dtoverlay=spi1-1cs,cs0_pin=16,cs0_spidev=disabled
dtoverlay=rcio
dtoverlay=navio-rgb

dtparam=i2c1=on
dtparam=i2c1_baudrate=1000000
```

Source:

[emlid/pi-gen-navio](https://github.com/emlid/pi-gen-navio/blob/navio-buster/stage3_emlid/03-kernel-tweaks/files/config.txt)*


## PWM Access Permissions

By default the Linux PWM interface is only accessible by the **root user**.  
Since the environment uses **rootless containers**, additional permissions must be configured.

Create a new udev rule:

```bash
sudo nano /etc/udev/rules.d/99-pwm.rules
```

Add the following rule:

```
SUBSYSTEM=="pwm", KERNEL=="pwmchip*", RUN+="/bin/sh -c 'chown -R root:users $(realpath /sys/class/pwm/%k) && chmod -R 770 $(realpath /sys/class/pwm/%k)'"
```

This allows users in the `users` group to access the PWM devices.


# Reboot

After completing the configuration reboot the system:

```bash
sudo reboot
```


# Verification

After reboot the RCIO module should be loaded automatically.

Verify with:

```bash
lsmod | grep rcio
```

If the driver is loaded successfully, the Navio2 hardware interfaces will be available to ROS nodes.