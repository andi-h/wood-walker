# System Setup

This directory documents the preparation of the base operating system on the Raspberry Pi used as the onboard computer of the platform.

## Real-time Ubuntu

The onboard computer runs:

**Ubuntu Server 24.04 LTS (64-bit)**

The official Raspberry Pi image provided by Canonical is used.

## Enable the Real-Time Kernel

Canonical provides a **real-time Linux kernel** for Ubuntu through the **Ubuntu Pro** subscription.

Ubuntu Pro is **free for up to five machines** and required to install the real-time kernel.

### Create an Ubuntu Account

Create an Ubuntu account:

https://login.ubuntu.com/

After logging in, obtain your **Ubuntu Pro token** from:

https://ubuntu.com/pro


### Attach the System to Ubuntu Pro

On the Raspberry Pi run:

```bash
sudo pro attach <YOUR_TOKEN>
```


### Install the Real-Time Kernel

Enable the real-time kernel for the Raspberry Pi platform:

```bash
sudo pro enable realtime-kernel --variant=raspi
```

Reboot the system after installation:

```bash
sudo reboot
```

## Update the System

Update the package lists and install the latest updates:

```bash
sudo apt update
sudo apt dist-upgrade
```

## Install Podman

All ROS 2 components of the platform run inside containers.  
The project uses **Podman** as container engine.

Install Podman using:

```bash
sudo apt install podman
```