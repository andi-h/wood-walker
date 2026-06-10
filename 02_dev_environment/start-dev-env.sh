#!/bin/bash

TOOLBOX_NAME="ros-dev-env"
ROS_DISTRO=humble

# Check if toolbox already exists
if toolbox list | grep -q "$TOOLBOX_NAME"; then
    echo "Toolbox $TOOLBOX_NAME already exists. Starting Visual Studio Code..."
else
    # Create a new toolbox
    echo "Creating a new toolbox: $TOOLBOX_NAME"

    SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
    podman build -t ros-toolbox --build-arg ROS_DISTRO=${ROS_DISTRO} $SCRIPT_DIR
    toolbox create -i ros-toolbox "$TOOLBOX_NAME"

    toolbox run --container "$TOOLBOX_NAME" env ROS_DISTRO=${ROS_DISTRO} bash <<EOF
    code --install-extension ms-vscode-remote.remote-containers

    systemctl --user enable podman.socket
    systemctl --user start podman.socket

    source /opt/ros/${ROS_DISTRO}/setup.bash

    # Build package timber_crane_description to use meshes in rviz2
    echo "Building timber_crane_description..."
    cd $SCRIPT_DIR/../03_vehicle_runtime
    colcon build --packages-select timber_crane_description
    source install/setup.bash

    # Build packages
    echo "Building RTABMAP..."
    cd $SCRIPT_DIR
    colcon build
    source install/setup.bash

    echo "Init rosdep..."
    rosdep init \
     && rosdep update --rosdistro ${ROS_DISTRO}  \
     && rosdep install --from-paths ./ -i -y --rosdistro ${ROS_DISTRO}

    echo "VS Code and Podman installation inside the toolbox is complete!"
EOF

    #toolbox run --container "$TOOLBOX_NAME"

echo "Toolbox $TOOLBOX_NAME setup complete! Starting Visual Studio Code..."
fi

toolbox run --container "$TOOLBOX_NAME" code ..
