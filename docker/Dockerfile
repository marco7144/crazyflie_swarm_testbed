# Base image: Official ROS 2 Jazzy Jalisco Desktop on Ubuntu 24.04 LTS (Noble) by OSRF.
# Pre-installs ROS 2 core middleware, standard messages, and RViz2.
FROM osrf/ros:jazzy-desktop

# Install core build tools, libraries, swig, and debugging utilities
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-pip \
    python3-dev \
    python3-vcstool \
    python-is-python3 \
    swig \
    sudo \
    curl \
    wget \
    nano \
    tmux \
    udev \
    usbutils \
    mesa-utils \
    libgl1-mesa-dri \
    libglx-mesa0 \
    # Step 2 from Crazyswarm2 official guide
    # Ref: https://imrclab.github.io/crazyswarm2/installation.html
    libboost-program-options-dev \
    libusb-1.0-0-dev \
    ros-jazzy-motion-capture-tracking \
    ros-jazzy-tf-transformations \
    && rm -rf /var/lib/apt/lists/*


# pip3 packaged installation for the python libraries in the step2 of the guide
# scipy is included for crazyflie_py swarm collision checking 
RUN pip3 install --no-cache-dir --break-system-packages --ignore-installed PyYAML \
    cflib \
    rowan \
    transforms3d \
    scipy \
    mujoco

# Install Crazyflow with GPU support (JAX with CUDA 12)
# Ref: https://github.com/learnsyslab/crazyflow
RUN pip3 install --no-cache-dir --break-system-packages "crazyflow[gpu]"

# Step 6 from Crazyswarm2 official guide (Software-in-the-loop simulation):
# Ref: https://imrclab.github.io/crazyswarm2/installation.html
# Pinned to release 2025.02 as recommended by Crazyswarm2 for maximum compatibility
RUN git clone --branch 2025.02 --single-branch --recursive \
        https://github.com/bitcraze/crazyflie-firmware.git /opt/crazyflie-firmware \
    && cd /opt/crazyflie-firmware \
    && make cf2_defconfig \
    && make include/generated/autoconf.h \
    && make python_wheel \
    && pip3 install --no-cache-dir --break-system-packages dist/*.whl

# Install Cyberbotics Webots & ROS 2 Jazzy simulation packages
# Ref: https://cyberbotics.com/doc/guide/installation-procedure
# Ref: https://docs.ros.org/en/jazzy/Tutorials/Advanced/Simulators/Webots/Installation-Ubuntu.html
RUN mkdir -p /etc/apt/keyrings \
    && wget -q https://cyberbotics.com/Cyberbotics.asc -O /etc/apt/keyrings/Cyberbotics.asc \
    && echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/Cyberbotics.asc] https://cyberbotics.com/debian binary-amd64/" > /etc/apt/sources.list.d/Cyberbotics.list \
    && apt-get update && apt-get install -y --no-install-recommends \
       xvfb \
       webots \
       ros-jazzy-webots-ros2 \
    && rm -rf /var/lib/apt/lists/*

ENV WEBOTS_HOME=/usr/local/webots
ENV PATH="${WEBOTS_HOME}:${PATH}"

# Install Open Robotics Gazebo Harmonic (Gz 8) & ROS 2 Jazzy integration
# Ref: https://gazebosim.org/docs/harmonic/install_ubuntu/
RUN curl https://packages.osrfoundation.org/gazebo.gpg --output /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] https://packages.osrfoundation.org/gazebo/ubuntu-stable $(lsb_release -cs) main" > /etc/apt/sources.list.d/gazebo-stable.list \
    && apt-get update && apt-get install -y --no-install-recommends \
       gz-harmonic \
       ros-jazzy-ros-gz \
    && rm -rf /var/lib/apt/lists/*

# Patch webots_ros2_driver to disable false-positive WSL check inside Linux container
RUN sed -i "s/'microsoft-standard' in uname().release/False/g" \
    /opt/ros/jazzy/lib/python3.12/site-packages/webots_ros2_driver/utils.py

# Set up non-root user matching the host UID/GID (prisma, 1000:1000)
# Note: Ubuntu 24.04 base image comes with a default 'ubuntu' user with UID 1000
ARG USERNAME=prisma
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Delete user if it exists in container (e.g. Ubuntu Noble: ubuntu)
# Ref: https://docs.ros.org/en/jazzy/How-To-Guides/Setup-ROS-2-with-VSCode-and-Docker-Container.html
RUN if id -u $USER_UID ; then userdel `id -un $USER_UID` ; fi \
    # Create the user
    && groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && usermod -aG dialout,plugdev,video,audio $USERNAME

ENV SHELL /bin/bash

# Copy entrypoint script
COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

# Configure automatic sourcing for interactive shells
RUN echo "source /opt/ros/jazzy/setup.bash" >> /home/$USERNAME/.bashrc \
    && echo "if [ -f /home/$USERNAME/ros2_ws/install/setup.bash ]; then source /home/$USERNAME/ros2_ws/install/setup.bash; fi" >> /home/$USERNAME/.bashrc

USER $USERNAME
WORKDIR /home/$USERNAME/ros2_ws

# Initialize and update rosdep cache for non-root user
RUN rosdep update

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
