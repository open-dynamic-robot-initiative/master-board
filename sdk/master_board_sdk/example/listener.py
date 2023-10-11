# coding: utf8

import argparse
import math
import os
import sys
try:
    from time import perf_counter
except ImportError:
    # You are still in python2… pleas upgrade :)
    from time import clock as perf_counter

import libmaster_board_sdk_pywrap as mbs


def listener_script(name_interface):

    cpt = 0  # Iteration counter
    dt = 0.001  #  Time step
    t = 0  # Current time

    print("-- Start of listener script --")

    os.nice(-20)  #  Set the process to highest priority (from -20 highest to +20 lowest)
    robot_if = mbs.MasterBoardInterface(name_interface, True)
    robot_if.Init()  # Initialization of the interface between the computer and the master board

    last = perf_counter()

    while (True):

        if ((perf_counter() - last) > dt):
            last = perf_counter()
            cpt += 1
            t += dt
            robot_if.ParseSensorData()  # Read sensor data sent by the masterboard

            if (cpt % 10000 == 0): # Reset stats every 10s
                robot_if.ResetPacketLossStats();

            if ((cpt % 100) == 0):  # Display state of the system once every 100 iterations of the main loop
                print(chr(27) + "[2J")
                # To read IMU data in Python use robot_if.imu_data_accelerometer(i), robot_if.imu_data_gyroscope(i)
                # or robot_if.imu_data_attitude(i) with i = 0, 1 or 2
                print("Session ID : {}".format(robot_if.GetSessionId()))
                robot_if.PrintIMU()
                robot_if.PrintADC()
                robot_if.PrintMotors()
                robot_if.PrintMotorDrivers()
                robot_if.PrintStats()
                sys.stdout.flush()  # for Python 2, use print( .... , flush=True) for Python 3

    print("-- End of example script --")


def main():
    parser = argparse.ArgumentParser(description='Listener for masterboard sensor packets in python.')
    parser.add_argument('-i',
                        '--interface',
                        required=True,
                        help='Name of the interface (use ifconfig in a terminal), for instance "enp1s0"')

    listener_script(parser.parse_args().interface)


if __name__ == "__main__":
    main()
