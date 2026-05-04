# WoodWalker

**WoodWalker** is a ROS 2 based laboratory platform built from a 1:14 scale RC timber truck equipped with a logging crane.  
The platform is designed for research and teaching in **mobile robotics**, **autonomous navigation**, and **robotic manipulation**.

The system integrates a Raspberry Pi based onboard computer, multiple sensors, and a modular ROS 2 software stack.  
Using **RTAB-Map** for SLAM and **Nav2** for navigation, the vehicle can create a map of its environment and autonomously navigate within it.

This repository documents the complete setup of the system, including operating system configuration, hardware drivers, containerized ROS environments, and development tooling.


# Platform Overview

The platform consists of two main computing environments:

**Onboard system (Raspberry Pi)**  
Runs low-level hardware interfaces and vehicle control.

**Workstation**  
Runs computationally intensive robotics algorithms such as SLAM and navigation.

```
(RPi)             (Workstation)                 (RPi)
Sensors   →   SLAM   →   Navigation → Vehicle Control
            (RTABMap)      (Nav2)      (ros2_control)
```

All ROS 2 components are executed inside **containers** to ensure reproducibility and simplify deployment.


# Hardware

## Base Vehicle

- **Tamiya 300023805 1:14 RC XB Volvo FH16 Timber Truck**
- 2 cell LiPo battery pack

## Logging Crane

Sicon-Modellbau short mast forestry crane  
https://www.sicon-modellbau.de/lade-und-forstkran-kurzmast-vormontiert-158.html

## Onboard Computer

- Raspberry Pi 4 (8 GB RAM)
- [Navio2 HAT](https://navio2.hipi.io/)
- Intel RealSense D435i depth camera
- Raspberry Pi Camera Module 3

The Navio2 board provides PWM outputs, IMU sensors, GNSS, and ADC inputs used for vehicle control and sensor integration.


# Software Architecture

The system is based on **ROS 2 Humble** and follows a modular architecture.

Key software components:

**Vehicle control**
- `ros2_control`
- custom Navio2 hardware interface
- bicycle steering controller

**SLAM**
- RTAB-Map
- RGB-D visual odometry

**Navigation**
- Nav2 navigation stack
- Hybrid-A* planner
- Regulated Pure Pursuit controller

**Visualization and debugging**
- Foxglove Studio
- RViz2
- rtabmap_viz


# Repository Structure

The repository is organized according to the different system layers.

```
00_system_setup
01_navio2_driver
02_dev_environment
03_vehicle_runtime
```

## 00_system_setup

Preparation of the base system on the Raspberry Pi.

Includes:

- Ubuntu 24.04 LTS installation
- real-time kernel setup
- installation of the container engine **Podman**

## 01_navio2_driver

Container environment used to build and install the **Navio2 RCIO kernel driver**.

The original [rcio](https://github.com/emlid/rcio-dkms) driver provided by Emlid is not compatible with modern kernels.  
This directory contains a migrated version compatible with **Linux kernel 6.8** and **64-bit systems**.

The driver is installed using **DKMS** so it can automatically rebuild when the kernel is updated.

## 02_dev_environment

Provides the **development and robotics environment** used on a workstation.

This container includes:

- ROS 2 desktop
- RTAB-Map
- Nav2 navigation stack
- visualization tools
- development utilities

It allows development and testing of ROS 2 nodes before deploying them to the runtime environment on the vehicle.

## 03_vehicle_runtime

Defines the **ROS 2 runtime container** executed on the Raspberry Pi.

This container runs the core onboard software:

- vehicle control nodes
- sensor interfaces
- hardware drivers
- ROS 2 runtime

The container is managed using **Podman** and started automatically through **systemd**.