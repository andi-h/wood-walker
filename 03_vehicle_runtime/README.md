# Vehicle Runtime Environment

This directory contains the **ROS 2 runtime environment executed on the Raspberry Pi** mounted on the vehicle.

The runtime environment is implemented as a **containerized ROS 2 workspace** and provides all components required for:

- vehicle control
- sensor integration
- robot description
- ROS communication

For development, the container is accessed remotely using **Visual Studio Code Dev Containers**.

## Requirements
 - Container engine installed on the host system (Docker, Podman, ...)
 - The ROS version can be set in `.devcontainer/Dockerfile` line 1. See [docker hub](https://hub.docker.com/_/ros) for possible tags. Also change the packages `ros-*-ros2-control` and `ros-*-ros2-controllers` in the Dockerfile.

## Development via Dev Containers

The runtime container is configured using:

```
.devcontainer/devcontainer.json
```

and built from:

```
.devcontainer/Dockerfile
```

The Dockerfile uses a **three-stage build process** to separate build dependencies from the final runtime image. For development, the second stage is used.

### Setup - Fedora
On *Fedora* [Toolbx](https://containertoolbx.org/) can be used to setup the development environment:
- Run `toolbx-env/start-dev-env.sh` in a terminal
- If successful, *Visual Studio Code* opens
- Press `Ctrl`+`,` and search for the setting `Dev Containers: Docker Path`
- Change the default value `docker` to `podman`

Containers can also be started locally for development and testing.

### Setup - Windows
Install *Visual Studio Code* with the *Remote Development* and the *Dev Container* extensions. Starting containers locally is possible, but out-of-scope of this documentation.

- Open *Visual Studio Code* and press `Ctrl`+`,` and search for the setting `Dev Containers: Docker Path`
- Change the default value `docker` to `podman`

### Connect
To connect to the runtime container:

1. Connect to the Raspberry Pi via SSH in **Visual Studio Code**
2. Open the folder `03_runtime_env`
3. Install the the *Dev Container* extension.
4. Run

```
View -> Command Palette... -> Dev Containers: Rebuild and Reopen in Container.
```

This launches the ROS runtime container on the Raspberry Pi.

## Navio2 Hardware Interface

The Navio2 board is accessed through the **RCIO kernel subsystem** provided by Emlid.

The RCIO driver registers a custom SPI driver that communicates with the microcontroller on the Navio2 board via the **SPI1 bus** of the Raspberry Pi.

Through this interface the following hardware components are exposed:

- PWM outputs for vehicle control
- ADC inputs
- IMU sensors
- GNSS receiver
- barometer

### PWM Interface

The PWM outputs are available through the Linux **sysfs PWM interface**:

```
/sys/class/pwm/pwmchip0/pwm[0-13]
```

Each PWM channel provides the following attributes:

```
enable
period
duty_cycle
```

These interfaces are used by the `truck_control` ROS package to generate actuator signals for:

- steering servo
- throttle controller
- additional actuators

#### PWM Failsafe Mechanism

To improve system safety, the RCIO driver implements a **heartbeat-based failsafe mechanism**.

The heartbeat interface is available at:

```
/sys/class/pwm/pwmchip0/device/heartbeat
```

The control software periodically updates this attribute.

If the heartbeat is not refreshed for more than **0.1 seconds**, the driver automatically disables all PWM outputs.

This ensures the vehicle enters a safe state if the control software crashes or becomes unresponsive.

### Sensor Interfaces

The sensors on the Navio2 board are connected through standard Linux device interfaces.

#### SPI Devices

Two IMUs and the GNSS module are connected via the **SPI0 bus** and are accessible through the following device files:

```
/dev/spidev0.3  LSM9DS1 accelerometer and gyroscope
/dev/spidev0.2  LSM9DS1 magnetometer
/dev/spidev0.1  MPU9250 9-DoF IMU
/dev/spidev0.0  u-blox M8N GNSS receiver
```

These devices are accessed by the Navio userspace libraries.

#### I²C Devices

The barometric pressure sensor **MS5611** is connected via the I²C bus:

```
/dev/i2c-1
```

#### ADC Interface

Analog inputs from the Navio board are exposed through the RCIO ADC interface:

```
/sys/kernel/rcio/adc
```

These values are published by the ROS node in the `navio` package.

Example measurements include:

- board voltage
- servo rail voltage
- power module voltage
- power module current

## ROS Workspace

All ROS packages are located in:

```
src/
```

Build the workspace with:

```bash
colcon build
source install/setup.bash
```

The robot models contained in this workspace are also used for visualization in the development environment.


## ROS Packages

The runtime environment includes the following ROS packages.

### intel_realsense

Provides integration of the **Intel RealSense D435i** camera.

Publishes:

- RGB images
- depth images
- IMU data

The node also starts the **Madgwick IMU filter** for orientation estimation.

Camera parameters such as resolution and frame rate can be configured in:

```
src/intel_realsense/launch/realsense_d435i_color.launch.py
```

#### Permissions
To set the right permissions to access the camera_node, copy the *udev* rule from [here](https://github.com/IntelRealSense/librealsense/blob/master/config/99-realsense-libusb.rules) to `/etc/udev/rules.d/99-realsense-libusb.rules` on the host system:
```bash
sudo curl -o /etc/udev/rules.d/99-realsense-libusb.rules https://raw.githubusercontent.com/IntelRealSense/librealsense/
refs/heads/master/config/99-realsense-libusb.rules
```
and add the udev rule `/etc/udev/rules.d/99-camera.rules`:
```
KERNEL=="video*", ACTION=="add", RUN+="/bin/sh -c 'chmod 770 /dev/%k'"
```

Reload udev with: 
```bash
sudo udevadm control --reload-rules && sudo udevadm trigger
```

### navio

Example ROS node demonstrating how to interface with the [Navio2 hardware drivers](https://github.com/emlid/Navio2/tree/master) from *emlid*.

Publishes sensor data from:

- IMU
- GNSS
- barometer
- ADC channels

This node is creadet according to the [ROS2 docs](https://ros2-tutorial.readthedocs.io/en/humble/cpp/cpp_library.html).

### navio_interfaces

Defines custom ROS messages used by the Navio nodes.

Example message:

```
float32 board_voltage
float32 servo_rail_voltage
float32 power_module_voltage
float32 power_module_current
float32 adc2_voltage
float32 adc3_voltage
```

### navio_py

Example Python implementation demonstrating how to access the Navio ADC interface.

Primarily intended for testing and demonstration purposes.

### timber_crane_description

Contains the URDF model and 3D meshes for the **timber crane**.

Used for:

- visualization
- robot description
- simulation

### truck_control

Main vehicle control package.

Provides:

- URDF model of the truck
- integration of the crane model
- `ros2_control` configuration
- custom Navio hardware interface

The vehicle is modeled using a **bicycle steering kinematic model**.

The implementation is based on the examples:
- Crane: https://github.com/ros-controls/ros2_control_demos/tree/master/example_4
- Truck: https://github.com/ros-controls/ros2_control_demos/tree/master/example_11
- GPIO: https://github.com/ros-controls/ros2_control_demos/tree/master/example_10

## Runtime Nodes

The following ROS nodes are started automatically inside the runtime container.

### Foxglove Bridge

Provides a WebSocket bridge for visualization tools.

```bash
ros2 launch foxglove_bridge foxglove_bridge_launch.xml
```

To connect, add new Foxglove WebSocket connection at https://app.foxglove.dev or the local Foxglove Studio desctbed in `../02_dev_environment`.

Also see the [Foxglove documentation](https://docs.foxglove.dev/docs/connecting-to-data/ros-foxglove-bridge/).

### Intel RealSense Camera

Starts the RealSense RGB-D camera pipeline.

```bash
ros2 launch intel_realsense realsense_d435i_color.launch.py
```

To start the ROS node without the launch file, run:
```bash
ros2 run realsense2_camera realsense2_camera_node --ros-args -p pointcloud.enable:=false
```
in the container.

## Starting the Runtime Container

There are two ways to start the runtime container on the Raspberry Pi.

---

### Option 1: Standard systemd Service

The runtime container can be started using a normal **systemd service**.

A reference service file is provided in:

```
systemd/truck.service
```

Typical usage:

1. Copy the service file to the systemd service directory
2. Reload systemd
3. Enable the service
4. Start the service

Example:

```bash
sudo cp systemd/truck.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable truck.service
sudo systemctl start truck.service
```

Check service status with:

```bash
systemctl status truck.service
```


### Option 2: Podman Quadlet

An alternative is to start the runtime container using **Podman Quadlet**.

A Quadlet container definition is provided in:

```
systemd/truck.container
```

In addition, a prepared build definition exists in:

```
systemd/truck.build
```

However, the `.build` extension is only supported starting with **Podman 5**.  
Ubuntu 24.04 currently ships with **Podman 4**, so this file cannot be used directly on the target system at the moment.

However, Quadlet can already be used for container startup via `truck.container`, which also handles image builds for now.

Typical usage:

Copy the Quadlet file to the user container configuration directory:

```bash
mkdir -p ~/.config/containers/systemd
cp systemd/truck.container ~/.config/containers/systemd/
```

Reload the user systemd configuration so that the Quadlet unit is detected:

```bash
systemctl --user daemon-reload
```

Start the container service:

```bash
systemctl --user start truck
```

To start the container automatically when the user logs in:

```bash
systemctl --user enable truck
```

Check service status:

```bash
systemctl --user status truck
```

Container logs can be viewed using:

```bash
journalctl --user -u truck -f
```

### Vehicle Control

Starts the ROS 2 control framework and loads the truck model.

```bash
ros2 launch truck_control truck.launch.py
```

### Navio Hardware Interface

Starts the Navio sensor interface node.

```bash
ros2 run navio truck_node
```

### Crane Camera

Starts the Raspberry Pi camera used to monitor the crane workspace. It uses the [camera_ros](https://github.com/christianrauch/camera_ros) node.

```bash
ros2 run camera_ros camera_node \
  --ros-args \
  -p camera:=0 \
  -p width:=640 \
  -p height:=480 \
  -p format:=RGB888
```

## Sensor Interfaces

The Navio2 hardware provides access to:

- PWM outputs for vehicle control
- IMU sensors
- GNSS receiver
- barometer
- ADC channels

These interfaces are exposed through the **RCIO kernel driver** installed in:

```
../01_navio2_driver
```


## Visualization

Robot models for the truck and crane are provided via URDF files and can be visualized in:

- RViz2
- Foxglove Studio
- RTAB-Map visualization tools

The development environment in `02_dev_environment` uses these models for visualization.

## Adding new ROS nodes
### Create ROS python nodes
A ROS demo package `python_demo_package` is already created in the `src` directory. To create a new package, run the following:
- `cd /workspaces/<PROJECT_NAME>`
- `mkdir src`
- `cd src`
- Note: this `src` folder is mapped to the local `src` folder when working locally.
- `ros2 pkg create --build-type ament_python --node-name <NODE_NAME> <PACKAGE_NAME>`
- Note: do not use hyphens (`-`) in the `<PACKAGE_NAME>`.
- Edit `<PACKAGE_NAME>/<PACKAGE_NAME>/<NODE_NAME>.py`.
- If more nodes are added, include it in `<PACKAGE_NAME>/setup.py` -> `entry_points` -> `console_scripts`.
- `cd /workspaces/<PROJECT_NAME>`
- `colcon build`
- `source install/setup.bash`
- `ros2 run <PACKAGE_NAME> <NODE_NAME>`
- To start multiple nodes, open separate terminals and run the last two commands.

Also see the ROS documentation [Create ROS2 Package](https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/Creating-Your-First-ROS2-Package.html) and [Write a Simple Python Node](https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Py-Publisher-And-Subscriber.html).

### Create ROS C++ nodes
- `cd /workspaces/<PROJECT_NAME>`
- `mkdir src`
- `cd src`
- Note: this `src` folder is mapped to the local `src` folder when working locally.
- `ros2 pkg create --build-type ament_cmake --node-name <NODE_NAME> <PACKAGE_NAME>`
- Note: do not use hyphens (`-`) in the `<PACKAGE_NAME>`.
- Edit `<PACKAGE_NAME>/src/<NODE_NAME>.cpp`.
- If more nodes are added, include it in `<PACKAGE_NAME>/CMakeLists.txt` after the first node (`add_executable(<NODE_NAME> src/<NODE_NAME>.cpp)`).
- `cd /workspaces/<PROJECT_NAME>`
- `colcon build`
- `source install/setup.bash`
- `ros2 run <PACKAGE_NAME> <NODE_NAME>`
- To start multiple nodes, open separate terminals and run the last two commands.

Also see the ROS documentation [Create ROS2 Package](https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/Creating-Your-First-ROS2-Package.html) and [Write a Simple C++ Node](https://docs.ros.org/en/humble/Tutorials/Beginner-Client-Libraries/Writing-A-Simple-Cpp-Publisher-And-Subscriber.html).