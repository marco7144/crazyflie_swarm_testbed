#!/usr/bin/env python3
"""
Server Crazyswarm2 (`crazyflie_sim`) con backend per Webots.

Collega il server ufficiale Crazyswarm2 (che esegue il pianificatore polinomiale C
di Bitcraze `cffirmware` tramite `CrazyflieSIL`) con il plugin C++ di Webots
(`crazyflie_webots_cpp::CrazyflieDriver`).
"""

from __future__ import annotations

from functools import partial
import math
import sys
import numpy as np

import cffirmware as firm
from rclpy.node import Node
from rclpy.time import Time
from geometry_msgs.msg import PoseStamped
from crazyflie_interfaces.msg import FullState, Position
from crazyflie_interfaces.srv import Arm
import crazyflie_sim.backend
from crazyflie_sim import sim_data_types
from crazyflie_sim.crazyflie_sil import CrazyflieSIL
from crazyflie_sim.crazyflie_server import main as crazyflie_server_main


class Backend:
    """Backend Crazyswarm2 che delega la dinamica fisica al plugin C++ di Webots."""

    def __init__(
        self,
        node: Node,
        names: list[str],
        states: list[sim_data_types.State],
    ):
        self._node = node
        self._names = list(names)
        self._t0_ns = node.get_clock().now().nanoseconds

        self._real_states: dict[str, sim_data_types.State] = {}
        self._last_pose_time: dict[str, Time | None] = {}
        self._setpoint_pubs = {}
        self._pose_subs = {}
        self._cmd_pos_subs = {}

        for name, init_state in zip(self._names, states):
            state_copy = sim_data_types.State()
            state_copy.pos = np.array(init_state.pos, dtype=float)
            state_copy.vel = np.zeros(3, dtype=float)
            state_copy.quat = np.array(init_state.quat, dtype=float)
            state_copy.omega = np.zeros(3, dtype=float)
            self._real_states[name] = state_copy
            self._last_pose_time[name] = None

            self._setpoint_pubs[name] = node.create_publisher(
                FullState,
                f'/{name}/setpoint_full_state',
                10,
            )

            # Legge direttamente la posa reale pubblicata dal driver Webots C++
            self._pose_subs[name] = node.create_subscription(
                PoseStamped,
                f'/{name}/pose',
                partial(self._on_pose, name=name),
                10,
            )

            self._cmd_pos_subs[name] = node.create_subscription(
                Position,
                f'/{name}/cmd_position',
                partial(self._on_cmd_position, name=name),
                10,
            )

            node.create_service(
                Arm,
                f'{name}/arm',
                partial(self._arm_callback, name=name),
            )

        node.create_service(Arm, 'all/arm', self._arm_callback)

        orig_emergency = getattr(node, '_emergency_callback', None)
        if orig_emergency is not None:
            def _wrapped_emergency(request, response, name='all'):
                cfs = node.cfs if name == 'all' else {name: node.cfs[name]}
                for _, cf in cfs.items():
                    cf.mode = CrazyflieSIL.MODE_IDLE
                    firm.plan_stop(cf.planner)
                return orig_emergency(request, response, name=name)
            node._emergency_callback = _wrapped_emergency

        self._node.get_logger().info(
            f'[WebotsBackend C++] Inizializzato per {len(self._names)} droni: {self._names}'
        )

    def _arm_callback(self, request, response, name: str = 'all'):
        self._node.get_logger().info(f'[{name}] arm(arm={request.arm})')
        return response

    def _on_cmd_position(self, msg: Position, name: str) -> None:
        if hasattr(self._node, 'cfs') and name in self._node.cfs:
            self._node.cfs[name].cmdFullState(
                [msg.x, msg.y, msg.z],
                [0.0, 0.0, 0.0],
                [0.0, 0.0, 0.0],
                math.radians(msg.yaw),
                [0.0, 0.0, 0.0],
            )

    def _on_pose(self, msg: PoseStamped, name: str) -> None:
        now = Time.from_msg(msg.header.stamp)
        new_pos = np.array([
            msg.pose.position.x,
            msg.pose.position.y,
            msg.pose.position.z,
        ], dtype=float)
        new_quat = np.array([
            msg.pose.orientation.w,
            msg.pose.orientation.x,
            msg.pose.orientation.y,
            msg.pose.orientation.z,
        ], dtype=float)

        state = self._real_states[name]
        last_time = self._last_pose_time[name]
        if last_time is not None:
            dt = (now.nanoseconds - last_time.nanoseconds) * 1e-9
            if 0.001 < dt < 0.5:
                state.vel = (new_pos - state.pos) / dt

        state.pos = new_pos
        state.quat = new_quat
        self._last_pose_time[name] = now

    def time(self) -> float:
        return (self._node.get_clock().now().nanoseconds - self._t0_ns) * 1e-9

    def step(
        self,
        states_desired: list[sim_data_types.State],
        actions: list[sim_data_types.Action],
    ) -> list[sim_data_types.State]:
        now_msg = self._node.get_clock().now().to_msg()
        cfs = getattr(self._node, 'cfs', {})

        for name, state_desired in zip(self._names, states_desired):
            cf = cfs.get(name)
            is_idle = False
            if cf is None or cf.mode == CrazyflieSIL.MODE_IDLE:
                is_idle = True
            elif (
                cf.mode == CrazyflieSIL.MODE_HIGH_POLY
                and firm.plan_is_stopped(cf.planner)
                and state_desired.pos[2] <= 0.05
            ):
                is_idle = True

            msg = FullState()
            msg.header.stamp = now_msg
            msg.header.frame_id = 'IDLE' if is_idle else 'world'

            msg.pose.position.x = float(state_desired.pos[0])
            msg.pose.position.y = float(state_desired.pos[1])
            msg.pose.position.z = float(state_desired.pos[2])

            msg.pose.orientation.w = float(state_desired.quat[0])
            msg.pose.orientation.x = float(state_desired.quat[1])
            msg.pose.orientation.y = float(state_desired.quat[2])
            msg.pose.orientation.z = float(state_desired.quat[3])

            msg.twist.linear.x = float(state_desired.vel[0])
            msg.twist.linear.y = float(state_desired.vel[1])
            msg.twist.linear.z = float(state_desired.vel[2])

            msg.twist.angular.x = float(state_desired.omega[0])
            msg.twist.angular.y = float(state_desired.omega[1])
            msg.twist.angular.z = float(state_desired.omega[2])

            if not is_idle and cf is not None and hasattr(cf, 'setpoint'):
                msg.acc.x = float(cf.setpoint.acceleration.x)
                msg.acc.y = float(cf.setpoint.acceleration.y)
                msg.acc.z = float(cf.setpoint.acceleration.z)

            self._setpoint_pubs[name].publish(msg)

        return [self._real_states[name] for name in self._names]

    def shutdown(self) -> None:
        now_msg = self._node.get_clock().now().to_msg()
        for name in self._names:
            msg = FullState()
            msg.header.stamp = now_msg
            msg.header.frame_id = 'IDLE'
            self._setpoint_pubs[name].publish(msg)


def main(args=None):
    sys.modules['crazyflie_sim.backend.webots'] = sys.modules[__name__]
    setattr(crazyflie_sim.backend, 'webots', sys.modules[__name__])
    crazyflie_server_main(args=args)


if __name__ == '__main__':
    main()
