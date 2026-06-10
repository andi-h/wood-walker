ARG ROS_DISTRO=humble
ARG ROS_DOMAIN_ID=52

FROM docker.io/ros:${ROS_DISTRO}-ros-core AS build

RUN apt-get update && \
     apt-get install -y --no-install-recommends \
     git \
     build-essential

WORKDIR /opt

RUN git clone https://github.com/joan2937/pigpio.git \
     && cd pigpio \
     && git checkout v79 \
     && make \
     && make install \
     && cd /opt

RUN apt-get update && \
     apt-get install -y --no-install-recommends \
     python3-pip \
     libboost-dev \
     libgnutls28-dev \
     openssl \
     libtiff-dev \
     pybind11-dev \
     qtbase5-dev \
     libqt5core5a \
     libqt5widgets5 \
     python3-yaml \
     libglib2.0-dev \
     libgstreamer-plugins-base1.0-dev \
     libudev-dev \
     libgstreamer1.0-dev \
     libpython3-dev pybind11-dev

RUN pip3 install --upgrade meson ninja jinja2 ply

RUN git clone https://github.com/raspberrypi/libcamera.git \
     && cd libcamera \
     && git checkout v0.7.0+rpt20260205 \
     && meson setup build --buildtype=release -Dpipelines=rpi/vc4,rpi/pisp -Dipas=rpi/vc4,rpi/pisp -Dv4l2=true -Dgstreamer=enabled -Dtest=false -Dlc-compliance=disabled -Dcam=disabled -Dqcam=disabled -Ddocumentation=disabled -Dpycamera=enabled \
     && ninja -C build install \
     && cd /opt

RUN apt-get update && \
     apt-get install -y --no-install-recommends \
     libboost-program-options-dev \
     libdrm-dev \
     libexif-dev \
     libpng-dev \
     && \
     : "remove cache" && \
     apt-get autoremove -y -qq && \
     rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/raspberrypi/rpicam-apps.git \
     && cd rpicam-apps \
     && git checkout v1.11.1 \
     && meson setup build -Denable_libav=disabled -Denable_drm=enabled -Denable_egl=disabled -Denable_qt=disabled -Denable_opencv=disabled -Denable_tflite=disabled -Denable_hailo=disabled \
     && meson compile -C build \
     && meson install -C build


FROM docker.io/ros:${ROS_DISTRO}-ros-core AS dev

ARG WORKSPACE_FOLDER
ARG ROS_DOMAIN_ID

COPY --from=build /usr/local /usr/local

RUN apt-get update \
     && apt-get install -y \
     nano \
     build-essential \
     git \
     gdb \
     python3-colcon-common-extensions \
     python3-spidev \
     python3-smbus \
     python3-rosdep \
     python3-pytest \
     liburdfdom-tools \
     ros-${ROS_DISTRO}-rmw-cyclonedds-cpp \
     ros-${ROS_DISTRO}-camera-ros \
     ros-${ROS_DISTRO}-librealsense2* \
     ros-${ROS_DISTRO}-realsense2-* \
     ros-${ROS_DISTRO}-image-transport \
     ros-${ROS_DISTRO}-compressed-image-transport \
     ros-${ROS_DISTRO}-compressed-depth-image-transport \
     ros-${ROS_DISTRO}-ros2-control \
     ros-${ROS_DISTRO}-ros2-controllers \
     ros-${ROS_DISTRO}-robot-state-publisher \
     ros-${ROS_DISTRO}-foxglove-bridge \
     ros-${ROS_DISTRO}-imu-filter-madgwick \
     ros-${ROS_DISTRO}-rtabmap-msgs \
     ros-${ROS_DISTRO}-nav2-msgs \
     ros-${ROS_DISTRO}-octomap-msgs \
     ros-${ROS_DISTRO}-dwb-msgs \
     ros-${ROS_DISTRO}-map-msgs \
     ros-${ROS_DISTRO}-bond \
     ros-${ROS_DISTRO}-rclpy \
     ros-${ROS_DISTRO}-ros2launch \
     ros-${ROS_DISTRO}-ament-flake8 \
     ros-${ROS_DISTRO}-ros2-controllers-test-nodes \
     ros-${ROS_DISTRO}-ament-pep257 \
     ros-${ROS_DISTRO}-pluginlib \
     ros-${ROS_DISTRO}-ament-copyright \
     ros-${ROS_DISTRO}-ament-cmake \
     ros-${ROS_DISTRO}-rosidl-default-runtime \
     ros-${ROS_DISTRO}-xacro \
     ros-${ROS_DISTRO}-launch-testing-ros \
     ros-${ROS_DISTRO}-ament-cmake-pytest \
     ros-${ROS_DISTRO}-force-torque-sensor-broadcaster \
     ros-${ROS_DISTRO}-joint-state-broadcaster \
     ros-${ROS_DISTRO}-rclcpp \
     ros-${ROS_DISTRO}-rosidl-default-generators \
     ros-${ROS_DISTRO}-hardware-interface \
     ros-${ROS_DISTRO}-ament-lint-common \
     ros-${ROS_DISTRO}-backward-ros \
     ros-${ROS_DISTRO}-forward-command-controller \
     ros-${ROS_DISTRO}-ros2controlcli \
     ros-${ROS_DISTRO}-ament-lint-auto \
     ros-${ROS_DISTRO}-std-msgs \
     ros-${ROS_DISTRO}-sensor-msgs \
     ros-${ROS_DISTRO}-rclcpp-lifecycle

COPY ./src ${WORKSPACE_FOLDER}/src

RUN cd ${WORKSPACE_FOLDER} \
     && rosdep init \
     && rosdep update --rosdistro ${ROS_DISTRO}  \
     && apt-get update \
     && rosdep install --from-paths ./ -i -y --rosdistro ${ROS_DISTRO} \
     --skip-keys ros-${ROS_DISTRO}-joint-state-publisher-gui --skip-keys ros-${ROS_DISTRO}-rviz2 \
     && \
     : "remove cache" && \
     apt-get autoremove -y -qq && \
     rm -rf /var/lib/apt/lists/*

RUN cd ${WORKSPACE_FOLDER} \
     && . /opt/ros/${ROS_DISTRO}/setup.sh \
     && colcon build

WORKDIR ${WORKSPACE_FOLDER}

RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> ~/.bashrc \
     && echo "source install/setup.bash" >> ~/.bashrc

COPY cyclonedds.xml /opt/ros/${ROS_DISTRO}/

ENV ROS_DOMAIN_ID ${ROS_DOMAIN_ID}
ENV ROS2_WS=${WORKSPACE_FOLDER}
ENV ROS_PYTHON_VERSION 3
ENV LD_LIBRARY_PATH /usr/local/lib:/usr/local/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ENV CYCLONEDDS_URI file:///opt/ros/${ROS_DISTRO}/cyclonedds.xml
ENV CYCLONEDDS_PORT_BASE=20410
ENV CYCLONEDDS_PORT_RANGE=90

FROM docker.io/ros:${ROS_DISTRO}-ros-core AS release

ARG WORKSPACE_FOLDER
ARG ROS_DOMAIN_ID

RUN apt-get update \
     && apt-get install -y --no-install-recommends \
     nano \
     python3-colcon-common-extensions \
     python3-spidev \
     python3-smbus \
     ros-${ROS_DISTRO}-rmw-cyclonedds-cpp \
     ros-${ROS_DISTRO}-camera-ros \
     ros-${ROS_DISTRO}-librealsense2* \
     ros-${ROS_DISTRO}-realsense2-* \
     ros-${ROS_DISTRO}-image-transport \
     ros-${ROS_DISTRO}-compressed-image-transport \
     ros-${ROS_DISTRO}-compressed-depth-image-transport \
     ros-${ROS_DISTRO}-ros2-control \
     ros-${ROS_DISTRO}-ros2-controllers \
     ros-${ROS_DISTRO}-robot-state-publisher \
     ros-${ROS_DISTRO}-foxglove-bridge \
     ros-${ROS_DISTRO}-imu-filter-madgwick \
     ros-${ROS_DISTRO}-rtabmap-msgs \
     ros-${ROS_DISTRO}-nav2-msgs \
     ros-${ROS_DISTRO}-octomap-msgs \
     ros-${ROS_DISTRO}-dwb-msgs \
     ros-${ROS_DISTRO}-map-msgs \
     ros-${ROS_DISTRO}-sensor-msgs \
     ros-${ROS_DISTRO}-bond \
     && \
     : "remove cache" && \
     apt-get autoremove -y -qq && \
     rm -rf /var/lib/apt/lists/*

COPY cyclonedds.xml /opt/ros/${ROS_DISTRO}/

ENV ROS_DOMAIN_ID ${ROS_DOMAIN_ID}
ENV ROS2_WS=${WORKSPACE_FOLDER}
ENV ROS_PYTHON_VERSION 3
ENV LD_LIBRARY_PATH /usr/local/lib:/usr/local/lib/aarch64-linux-gnu:$LD_LIBRARY_PATH
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ENV CYCLONEDDS_URI file:///opt/ros/${ROS_DISTRO}/cyclonedds.xml
ENV CYCLONEDDS_PORT_BASE=20410
ENV CYCLONEDDS_PORT_RANGE=90

COPY --from=build /usr/local /usr/local
COPY --from=dev ${WORKSPACE_FOLDER}/install ${WORKSPACE_FOLDER}/install
COPY --from=dev ${WORKSPACE_FOLDER}/src ${WORKSPACE_FOLDER}/src

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> ~/.bashrc \
     && echo "source install/setup.bash" >> ~/.bashrc

WORKDIR ${WORKSPACE_FOLDER}

ENTRYPOINT ["/entrypoint.sh"]
