#!/bin/bash
# Step 3.1 from Crazyswarm2 official guide
# Ref: https://imrclab.github.io/crazyswarm2/installation.html
mkdir -p ros2_ws/src

if [ ! -d "ros2_ws/src/crazyswarm2" ]; then
    echo "Cloning crazyswarm2 repository."
    git clone --recursive -b main https://github.com/IMRCLab/crazyswarm2.git ros2_ws/src/crazyswarm2
else
    echo "crazyswarm2 is already present."
fi

echo "Workspace setup completed successfully."
