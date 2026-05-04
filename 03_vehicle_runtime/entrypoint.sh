#!/usr/bin/env bash
set -eo pipefail

source "/opt/ros/${ROS_DISTRO}/setup.bash"
source "${ROS2_WS}/install/setup.bash"

# Forward stop signals to child processes and wait for clean exit
pids=()
shutdown() {
  echo "entrypoint: shutting down..."
  # Send SIGINT first (ROS launch tends to handle it nicely), then SIGTERM
  for pid in "${pids[@]}"; do kill -INT  "$pid" 2>/dev/null || true; done
  sleep 2
  for pid in "${pids[@]}"; do kill -TERM "$pid" 2>/dev/null || true; done
  wait || true
}
trap shutdown SIGINT SIGTERM

ros2 launch foxglove_bridge foxglove_bridge_launch.xml &
pids+=($!)

ros2 launch intel_realsense realsense_d435i_color.launch.py &
pids+=($!)

ros2 launch truck_control truck.launch.py &
pids+=($!)

ros2 run navio truck_node &
pids+=($!)

ros2 run camera_ros camera_node --ros-args -p camera:=0 -p width:=640 -p height:=480 -p format:=RGB888 &
pids+=($!)

# Keep container alive until one of the launches exits; then shut down the rest
wait -n "${pids[@]}"
rc=$?
echo "entrypoint: a launch exited (rc=$rc), stopping others..."
shutdown
exit "$rc"
