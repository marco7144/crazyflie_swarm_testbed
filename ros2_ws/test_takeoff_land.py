#!/usr/bin/env python3
"""
Test minimale di decollo, stazionamento (hovering) e atterraggio.
Funziona sia con 1 singolo drone che con lo sciame di 4 droni tramite Crazyswarm2.
"""

from crazyflie_py import Crazyswarm


def main():
    # Inizializza Crazyswarm2 (legge automaticamente tutti i droni attivi)
    swarm = Crazyswarm()
    timeHelper = swarm.timeHelper
    allcfs = swarm.allcfs

    flight_height = 0.6     # Quota di volo in metri
    takeoff_duration = 2.5  # Durata della salita in secondi
    hover_duration = 5.0    # Secondi in cui rimanere fermi in aria
    land_duration = 2.5     # Durata della discesa in secondi

    # 1. Decollo verticale
    print(f"1) Decollo a quota {flight_height} m...")
    allcfs.takeoff(targetHeight=flight_height, duration=takeoff_duration)
    timeHelper.sleep(takeoff_duration)

    # 2. Stazionamento in quota (Hovering)
    print(f"2) Fermo in quota per {hover_duration} secondi...")
    timeHelper.sleep(hover_duration)

    # 3. Atterraggio verticale dolce
    print("3) Atterraggio...")
    allcfs.land(targetHeight=0.02, duration=land_duration)
    timeHelper.sleep(land_duration + 2.0)

    print("Completato!")


if __name__ == '__main__':
    main()

