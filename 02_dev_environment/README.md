# Development Environment

This directory contains the ROS 2 development environment used on the **workstation**.

The environment is intended for:

- development of ROS 2 packages
- running **RTAB-Map**
- running **Nav2**
- visualization and debugging

This environment is executed inside a container managed by **Toolbx** on the workstation.


## Overview

The workstation environment provides a full ROS 2 desktop setup including:

- ROS 2 Humble
- RTAB-Map
- Nav2
- RViz2
- Foxglove Studio
- Foxglove bridge support
- build tools such as `colcon`

This environment is used for computationally intensive robotics tasks such as SLAM and navigation.

---

## Container Runtime

On Fedora systems the development environment is typically started using **Toolbx**.

Toolbx provides an interactive development container integrated into the host desktop environment while keeping the ROS setup isolated from the host system.

The workspace in this directory is built and executed inside that Toolbx container.

It can be started by executing `./start-dev-env.sh`, which automatically starts VS Code within the container.

To rebuild the container, remove it first with `toolbox rm -f ros-dev-env`.


## ROS Workspace

All ROS packages are located in:

```
src/
```

Typical workflow:

```bash
colcon build
source install/setup.bash
```

Before running any ROS nodes in this environment, always ensure that the workspace has been built and sourced.

## Robot Model Visualization

Before running visualization tools such as RViz2, RTAB-Map visualization, or Foxglove, make sure the runtime workspace has been built as well, so that the truck and crane URDF models are available:

```bash
cd ../03_runtime_env
colcon build
source install/setup.bash
```

This ensures that the 3D models of the truck and crane are available for visualization.


## Running RTAB-Map and Navigation

To start RTAB-Map and Nav2, run:

```bash
cd 02_dev_environment

colcon build
source install/setup.bash
source /opt/ros/${ROS_DISTRO}/setup.bash

ros2 launch rtabmap_launch rtabmap_realsense.launch.py
```

This launch file starts the mapping and localization pipeline based on RGB-D data and IMU measurements.

The RTAB-Map and Nav2 configuration can be modified in:

```
src/rtabmap_launch/launch/rtabmap_realsense.launch.py
src/rtabmap_launch/params/nav2_params.yaml
```

## Foxglove Bridge

To expose ROS data to Foxglove Studio, start the bridge with:

```bash
ros2 launch foxglove_bridge foxglove_bridge_launch.xml
```

Then start Foxglove Studio:

```bash
foxglove-studio
```


## Remote Development on the Raspberry Pi

Remote development on the Raspberry Pi is handled separately via the runtime environment in:

```
../03_runtime_env
```

There, **Visual Studio Code Dev Containers** are used remotely on the Raspberry Pi to work inside the runtime container.

This means:

- **02_dev_environment** → local workstation container via **Toolbx**
- **03_runtime_env** → remote development on Raspberry Pi via **Dev Containers**


## Notes

This environment is intended for development and high-level robotics functionality.  
Low-level hardware access and vehicle control are handled in the runtime environment on the Raspberry Pi.