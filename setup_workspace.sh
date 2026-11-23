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

#Cloning Crazyflow repository
if [ ! -d "crazyflow" ]; then
    echo "Cloning crazyflow repository."
    git clone https://github.com/learnsyslab/crazyflow.git crazyflow
else
    echo "crazyflow is already present."
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

# Configurazione permessi USB:
# Ref: https://www.bitcraze.io/documentation/repository/crazyflie-lib-python/0.1.34/installation/usb_permissions/
UDEV_RULE_FILE="/etc/udev/rules.d/99-bitcraze.rules"
if [ ! -f "$UDEV_RULE_FILE" ]; then
    echo "Configurazione permessi USB Bitcraze su host Linux"
    sudo groupadd -f plugdev
    sudo usermod -a -G plugdev "$USER"
    cat <<EOF | sudo tee "$UDEV_RULE_FILE" > /dev/null
# Crazyradio (normal operation)
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="7777", MODE="0664", GROUP="plugdev"
# Bootloader
SUBSYSTEM=="usb", ATTRS{idVendor}=="1915", ATTRS{idProduct}=="0101", MODE="0664", GROUP="plugdev"
# Crazyflie (over USB)
SUBSYSTEM=="usb", ATTRS{idVendor}=="0483", ATTRS{idProduct}=="5740", MODE="0664", GROUP="plugdev"
EOF
    sudo udevadm control --reload-rules && sudo udevadm trigger
    echo "Regole udev Bitcraze installate con successo."
    echo "NOTA: Se è la prima volta che configuri la Crazyradio, effettua Logout e Login (o riavvia il PC) per rendere effettivi i permessi del gruppo plugdev."
else
    echo "Regole udev Bitcraze già presenti ($UDEV_RULE_FILE)."
fi

echo "Workspace setup completed successfully."
