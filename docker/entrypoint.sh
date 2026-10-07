#!/bin/bash
set -e

# Setup clean runtime directory for Qt/GUI without permission warnings
export XDG_RUNTIME_DIR="/tmp/runtime-${USER:-prisma}"
mkdir -p "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"

# Source ROS 2 Jazzy underlay
if [ -f "/opt/ros/jazzy/setup.bash" ]; then
    source "/opt/ros/jazzy/setup.bash"
fi

# Source workspace if it has been built overlay
if [ -f "${HOME}/ros2_ws/install/setup.bash" ]; then
    source "${HOME}/ros2_ws/install/setup.bash"
fi

exec "$@"
