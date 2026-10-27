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

#Cloning CrazySim repository
if [ ! -d "CrazySim" ]; then
    echo "Cloning CrazySim repository."
    git clone --recursive https://github.com/gtfactslab/CrazySim.git CrazySim
else
    echo "CrazySim is already present."
fi

#fix for 64 bit systems
if [ -d "CrazySim/crazyflie-firmware" ] && [ -f "patches/crazysim_64bit_fix.patch" ]; then
    echo "Verifica patch"
    cd CrazySim/crazyflie-firmware
    if git apply --check ../../patches/crazysim_64bit_fix.patch 2>/dev/null; then
        git apply ../../patches/crazysim_64bit_fix.patch
        echo "Patch 64-bit applicata con successo."
    else
        echo "Patch gia' applicata o non necessaria."
    fi
    cd ../..
fi

#building crazyflie-firmware
if [ -d "CrazySim/crazyflie-firmware" ] && [ ! -f "CrazySim/crazyflie-firmware/sitl_make/build/cf2" ]; then
    echo "Compilazione firmware SITL (cf2)..."
    mkdir -p CrazySim/crazyflie-firmware/sitl_make/build
    cd CrazySim/crazyflie-firmware/sitl_make/build
    cmake ..
    make -j$(nproc) all
    cd ../../../..
    echo "Firmware SITL (cf2) compilato con successo."
fi

echo "Workspace setup completed successfully."
