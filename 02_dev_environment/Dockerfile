# Copied and changed from https://github.com/toolbx-images/images/blob/main/debian/12/Containerfile
ARG ROS_DISTRO

FROM docker.io/osrf/ros:${ROS_DISTRO}-desktop-full

LABEL com.github.containers.toolbox="true" \
      name="ros-dev-env" \
      version="${ROS_DISTRO}" \
      usage="This image is meant to be used with the toolbox command" \
      summary="Base image for creating ROS toolbox containers" \
      maintainer=""

# Remove apt configuration optimized for containers
# Remove docker-gzip-indexes to help with "command-not-found"
RUN rm /etc/apt/apt.conf.d/docker-gzip-indexes /etc/apt/apt.conf.d/docker-no-languages

# Enable myhostname nss plugin for clean hostname resolution without patching
# hosts (at least for sudo), add it right after 'files' entry. We expect that
# this entry is not present yet. Do this early so that package postinst (which
# adds it too late in the order) skips this step
RUN sed -Ei 's/^(hosts:.*)(\<files\>)\s*(.*)/\1\2 myhostname \3/' /etc/nsswitch.conf

# Prevent questions when installing packages
ARG DEBIAN_FRONTEND=noninteractive

# Make flatpak-xdg-utils usable inside the toolbox
COPY toolbox-flatpak-xdg-utils.sh /etc/profile.d

# Install extra packages as well as libnss-myhostname
COPY extra-packages /

# Workaround: https://github.com/ros2/ros2/issues/1696#issuecomment-2930476182
#RUN rm /etc/apt/sources.list.d/ros2-latest.list || true && \
#    apt-get update && apt-get install -y curl && \
#    curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg && \
#    echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo $UBUNTU_CODENAME) main" | sudo tee /etc/apt/#sources.list.d/ros2.list > /dev/null

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get -y install \
        libnss-myhostname \
        wget \
        gpg \
        ros-${ROS_DISTRO}-rmw-cyclonedds-cpp \
        ros-${ROS_DISTRO}-compressed-image-transport \
        ros-${ROS_DISTRO}-compressed-depth-image-transport \
        ros-${ROS_DISTRO}-rtabmap-rviz-plugins \
        ros-${ROS_DISTRO}-rtabmap-viz \
        ros-${ROS_DISTRO}-navigation2 \
        ros-${ROS_DISTRO}-nav2-bringup \
        ros-${ROS_DISTRO}-turtlebot3* \
        ros-${ROS_DISTRO}-rtabmap-ros \
        ros-${ROS_DISTRO}-foxglove-bridge \
        $(cat extra-packages | xargs)
RUN rm /extra-packages

# Install VS Code
RUN wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg \
    && sudo install -D -o root -g root -m 644 packages.microsoft.gpg /etc/apt/keyrings/packages.microsoft.gpg \
    && echo "deb [arch=amd64,arm64,armhf signed-by=/etc/apt/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" |sudo tee /etc/apt/sources.list.d/vscode.list > /dev/null \
    && rm -f packages.microsoft.gpg

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get -y install \
        code

# Install foxglove
RUN wget -O /tmp/foxglove-studio.deb "https://get.foxglove.dev/desktop/latest/foxglove-studio-latest-linux-amd64.deb" && \
    sudo apt install -y /tmp/foxglove-studio.deb && \
    rm /tmp/foxglove-studio.deb && \
    rm -rd /var/lib/apt/lists/*

# Install podman-remote
RUN PODMAN_VERSION=$(curl -s https://api.github.com/repos/containers/podman/releases/latest | grep "tag_name" | cut -d '"' -f 4) && \
	wget https://github.com/containers/podman/releases/download/${PODMAN_VERSION}/podman-remote-static-linux_amd64.tar.gz && \
	mkdir podman-remote && \
	tar -xvf podman-remote-static-linux_amd64.tar.gz -C podman-remote && \
	sudo mv podman-remote/bin/podman-remote-static-linux_amd64 /usr/local/bin/podman-remote && \
	sudo chmod +x /usr/local/bin/podman-remote

# Enable password less sudo
RUN sed -i -e 's/ ALL$/ NOPASSWD:ALL/' /etc/sudoers

RUN echo VARIANT_ID=container >> /etc/os-release

RUN touch /etc/localtime

COPY cyclonedds.xml /opt/ros/${ROS_DISTRO}/

ENV ROS_DOMAIN_ID 52
ENV CYCLONEDDS_URI file:///opt/ros/${ROS_DISTRO}/cyclonedds.xml
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> /etc/bash.bashrc \
    && echo "source install/setup.bash" >> /etc/bash.bashrc

# Empty /home to be compatible with toolbx
RUN rm -rf /home/*
