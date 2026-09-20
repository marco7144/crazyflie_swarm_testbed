# Crazyflie Swarm Testbed (ROS 2 Jazzy & Mixed-Reality)

Testbed sperimentale e di simulazione per uno sciame di Crazyflie

## Avvio Rapido

### 0. Download dipendenze esterne
Se cloni il repository su un nuovo computer:
```bash
cd crazyflie_swarm_testbed
./setup_workspace.sh
```

### 1. Build dell'immagine Docker
* **Windows 11 (WSL2):**
  ```bash
  docker compose build
  ```
* **Linux Nativo:**
  ```bash
  docker compose -f docker-compose.linux.yml build
  ```

### 2. Avvio del container
* **Windows 11 (WSL2):**
  ```bash
  docker compose up -d
  ```
* **Linux Nativo:**
  ```bash
  docker compose -f docker-compose.linux.yml up -d
  ```

### 3. Accesso alla shell del container
* **Windows 11 (WSL2):**
  ```bash
  docker compose exec swarm-dev bash
  ```
* **Linux Nativo:**
  ```bash
  docker compose -f docker-compose.linux.yml exec swarm-dev bash
  ```
* **Comando rapido universale (funziona sia su Windows che su Linux):**
  ```bash
  docker exec -it crazyflie_swarm_dev bash
  ```

### 4. Compilazione del Workspace ROS 2
All'interno del container (da eseguire la prima volta o dopo modifiche al codice):
```bash
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