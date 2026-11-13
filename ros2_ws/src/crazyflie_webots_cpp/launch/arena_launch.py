"""
Launch file per lo sciame da 4 droni (cf_01..cf_04) nell'Arena PRISMA in Webots
usando il plugin C++ (crazyflie_webots_cpp::CrazyflieDriver) e Crazyswarm2.

Supporta il parametro `crazyflies_yaml_file` (come il launch ufficiale di Crazyswarm2)
per condividere un unico file YAML di configurazione con CrazySim (MuJoCo / Gazebo).

Uso:
  Terminale 1:
    ros2 launch crazyflie_webots_cpp arena_launch.py crazyflies_yaml_file:=/home/prisma/ros2_ws/crazyflies_crazysim.yaml
  Terminale 2:
    python3 /home/prisma/ros2_ws/test_swarm_crazyswarm2.py
"""

import os
import yaml
import launch
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
from webots_ros2_driver.webots_launcher import WebotsLauncher
from webots_ros2_driver.webots_controller import WebotsController


def launch_setup(context):
    pkg_dir = get_package_share_directory('crazyflie_webots_cpp')
    cf_pkg_dir = get_package_share_directory('crazyflie')
    cf_desc_dir = get_package_share_directory('crazyflie_description')

    world_path = os.path.join(pkg_dir, 'worlds', 'arena_prisma.wbt')
    webots_urdf_path = os.path.join(pkg_dir, 'resource', 'crazyflie_webots.urdf')
    cf_urdf_path = os.path.join(cf_desc_dir, 'urdf', 'crazyflie_description.urdf')

    crazyflies_yaml_path = LaunchConfiguration('crazyflies_yaml_file').perform(context)
    server_yaml_path = os.path.join(pkg_dir, 'config', 'server_sim.yaml')

    with open(crazyflies_yaml_path, 'r') as f:
        crazyflies_cfg = yaml.safe_load(f)
    with open(server_yaml_path, 'r') as f:
        server_cfg = yaml.safe_load(f)['/crazyflie_server']['ros__parameters']
    with open(cf_urdf_path, 'r') as f:
        server_cfg['robot_description'] = f.read()

    # 1. Simulatore fisico Webots
    webots = WebotsLauncher(
        world=world_path,
        ros2_supervisor=True,
    )

    # 2. Controller fisici Webots C++ per i droni abilitati nel file YAML
    drone_names = [
        name for name, robot in crazyflies_cfg.get('robots', {}).items()
        if robot.get('enabled', True)
    ]
    controllers = []
    for name in drone_names:
        controllers.append(
            WebotsController(
                robot_name=name,
                parameters=[{'robot_description': webots_urdf_path}],
                respawn=True,
            )
        )

    # 3. Server ufficiale Crazyswarm2 con backend Webots
    crazyflie_server = Node(
        package='crazyflie_webots_cpp',
        executable='webots_crazyflie_server.py',
        name='crazyflie_server',
        output='screen',
        parameters=[server_cfg, crazyflies_cfg],
    )

    # 4. RViz2 opzionale
    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', os.path.join(cf_pkg_dir, 'config', 'config.rviz')],
        parameters=[{'use_sim_time': True}],
        condition=IfCondition(LaunchConfiguration('rviz')),
    )

    return [
        webots,
        webots._supervisor,
        *controllers,
        crazyflie_server,
        rviz_node,
        launch.actions.RegisterEventHandler(
            event_handler=launch.event_handlers.OnProcessExit(
                target_action=webots,
                on_exit=[launch.actions.EmitEvent(event=launch.events.Shutdown())],
            )
        ),
    ]


def generate_launch_description():
    pkg_dir = get_package_share_directory('crazyflie_webots_cpp')
    shared_crazysim_yaml = '/home/prisma/ros2_ws/crazyflies_crazysim.yaml'
    default_crazyflies_yaml = (
        shared_crazysim_yaml
        if os.path.isfile(shared_crazysim_yaml)
        else os.path.join(pkg_dir, 'config', 'crazyflies_swarm_sim.yaml')
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            'crazyflies_yaml_file',
            default_value=default_crazyflies_yaml,
            description='Percorso del file YAML unico di configurazione dello sciame',
        ),
        DeclareLaunchArgument(
            'rviz',
            default_value='false',
            description='Avvia anche RViz2 oltre alla finestra 3D di Webots',
        ),
        OpaqueFunction(function=launch_setup),
    ])
