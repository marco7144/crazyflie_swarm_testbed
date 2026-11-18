# Crazyflie Swarm Testbed (ROS 2 Jazzy & Mixed-Reality)

Testbed sperimentale e di simulazione per uno sciame di Crazyflie

## Prerequisiti di Sistema (Host)
1. **Docker Engine** e **Docker Compose** installati.
2. *(Opzionale, consigliato per GPU NVIDIA)*: Driver ufficiali NVIDIA e [NVIDIA Container Toolkit](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html).

## Avvio Rapido

### 0. Download dipendenze esterne
Se cloni il repository su un nuovo computer:
```bash
cd crazyflie_swarm_testbed
./setup_workspace.sh
```

### 1. Build dell'immagine Docker
* **PC con GPU NVIDIA:**
```bash
docker compose build
```
* **PC con GPU Intel / AMD Radeon:**
```bash
docker compose -f docker-compose.intel-amd.yml build
```

### 2. Avvio del container
* **PC con GPU NVIDIA:**
```bash
docker compose up -d
```
* **PC con GPU Intel / AMD Radeon:**
```bash
docker compose -f docker-compose.intel-amd.yml up -d
```

### 3. Accesso alla shell del container
```bash
docker compose exec swarm-dev bash
```
oppure:
```bash
docker exec -it crazyflie_swarm_dev bash
```

### 4. Compilazione all'interno del container
All'interno del container (da eseguire la prima volta o dopo modifiche al codice):

* **Compilazione firmware SITL CrazySim (cf2):**
  ```bash
  mkdir -p /home/prisma/CrazySim/crazyflie-firmware/sitl_make/build
  cd /home/prisma/CrazySim/crazyflie-firmware/sitl_make/build
  cmake .. && make -j$(nproc) all
  ```

* **Compilazione Workspace ROS 2:**
  ```bash
  cd /home/prisma/ros2_ws
  sudo apt update
  rosdep install --from-paths src --ignore-src -y
  colcon build --symlink-install
  source install/setup.bash
  ```

### 5. Test e Avvio Simulazioni
All'interno del container:

* **Test Grafica RViz2:**
  ```bash
  rviz2
  ```

* **Test Volo Simulato Crazyflie (Crazyswarm2 SITL):**
  ```bash
  ros2 launch crazyflie_examples launch.py script:=hello_world backend:=sim mocap:=False teleop:=False rviz:=True
  ```

* **Test Volo Crazysim - MuJoCo**
  - **Avvio firmware Crazyflie** (modelli supportati: `cf2x_L250`, `cf2x_T350`, `cf2x_P250`, `cf21B_500`)
    - Modello **Crazyflie 2.x Standard** (`cf2x_L250`):
      ```bash
      cd /home/prisma/CrazySim/crazyflie-firmware
      bash tools/crazyflie-simulation/simulator_files/mujoco/launch/sitl_multiagent_square.sh -n 4 -m cf2x_L250
      ```
    - Modello **Crazyflie 2.1 Brushless** (`cf21B_500`):
      ```bash
      cd /home/prisma/CrazySim/crazyflie-firmware
      bash tools/crazyflie-simulation/simulator_files/mujoco/launch/sitl_multiagent_square.sh -n 4 -m cf21B_500
      ```
  - **In un secondo terminale del container**
    ```bash
    source install/setup.bash
    ros2 launch crazyflie launch.py backend:=cflib mocap:=False teleop:=False crazyflies_yaml_file:=/home/prisma/ros2_ws/crazyflies_crazysim.yaml
    ```
  - **In un terzo terminale del container**
    ```bash
    python3 /home/prisma/ros2_ws/test_swarm_crazyswarm2.py
    ```

* **Test Volo Crazysim - Gazebo**
  - **Avvio firmware Crazyflie**
    ```bash
    cd /home/prisma/CrazySim/crazyflie-firmware
    bash tools/crazyflie-simulation/simulator_files/gazebo/launch/sitl_multiagent_square.sh -n 4
    ```
  - **In un secondo terminale del container**
    ```bash
    source install/setup.bash
    ros2 launch crazyflie launch.py backend:=cflib mocap:=False teleop:=False crazyflies_yaml_file:=/home/prisma/ros2_ws/crazyflies_crazysim.yaml
    ```
  - **In un terzo terminale del container**
    ```bash
    python3 /home/prisma/ros2_ws/test_swarm_crazyswarm2.py
    ```

* **Test Volo Webots**
  - **Avvio simulatore Webots + Plugin C++ + Crazyswarm2 Server (4 droni)**
    ```bash
    source install/setup.bash
    ros2 launch crazyflie_webots_cpp arena_launch.py crazyflies_yaml_file:=/home/prisma/ros2_ws/crazyflies_crazysim.yaml
    ```
  - **In un secondo terminale del container (script compatibili anche con Crazysim)**
    ```bash
    source install/setup.bash
    python3 /home/prisma/ros2_ws/test_swarm_crazyswarm2.py
    ```
    Oppure test decollo/atterraggio:
    ```bash
    source install/setup.bash
    python3 /home/prisma/ros2_ws/test_takeoff_land.py
    ```

* **Test Volo Crazyflow (MuJoCo)**
  - **Esempi ufficiali (1 drone in hover / sciame di 4 droni a spirale)**
    ```bash
    python3 /home/prisma/crazyflow/examples/control/hover.py
    ```
    oppure:
    ```bash
    python3 /home/prisma/crazyflow/examples/control/spiral.py
    ```
  *(Di default gli esempi usano il Crazyflie 2.1 Brushless `cf21B_500`; negli script è possibile selezionare altri modelli come `cf2x_L250` tramite `Sim(drone=...)`. Altri esempi ufficiali sono disponibili in `/home/prisma/crazyflow/examples/`.)*