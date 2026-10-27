#!/usr/bin/env python3

from crazyflie_py import Crazyswarm


def main():
    swarm = Crazyswarm()
    timeHelper = swarm.timeHelper
    allcfs = swarm.allcfs

    # 1. Decollo dello sciame 
    flight_height = 0.6
    takeoff_duration = 2.5
    print(f"1) Decollo dei droni")
    allcfs.takeoff(targetHeight=flight_height, duration=takeoff_duration)
    timeHelper.sleep(takeoff_duration + 2.0)

    # 2. Spostamento su asse X
    print("2) goTo a +0.5m su x")
    allcfs.goTo(goal=[0.5, 0.0, 0.0], yaw=0.0, duration=3.0)
    timeHelper.sleep(4.0)

    # 3. Spostamento su asse Y
    print("3) goTo a +0.5m su y")
    allcfs.goTo(goal=[0.0, 0.5, 0.0], yaw=0.0, duration=3.0)
    timeHelper.sleep(4.0)

    # 4. Ritorno alla posizione originaria
    print("4) goTo a -0.5m su x e -0.5m su y")
    allcfs.goTo(goal=[-0.5, -0.5, 0.0], yaw=0.0, duration=3.0)
    timeHelper.sleep(4.0)

    # 5. Atterraggio dello sciame
    land_duration = 2.5
    print(f"5) Atterraggio dei droni")
    allcfs.land(targetHeight=0.02, duration=land_duration)
    timeHelper.sleep(land_duration + 2.0)


if __name__ == '__main__':
    main()

