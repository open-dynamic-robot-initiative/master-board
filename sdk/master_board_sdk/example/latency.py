# coding: utf8

import argparse
import math
import os
import sys
from time import clock

import libmaster_board_sdk_pywrap as mbs
import matplotlib.pyplot as plt


def example_script(name_interface):

    N_SLAVES = 6  #  Maximum number of controled drivers
    N_SLAVES_CONTROLED = 6  # Current number of controled drivers

    cpt = 0
    dt = 0.001  #  Time step
    state = 0  # State of the system (ready (1) or not (0))
    duration = 30 # Duration in seconds

    if (duration == 0) :
        print("Null duration selected, end of script")
        return

    sent_list = [0.0 for i in range(int(duration*1000))]
    received_list = [0.0 for i in range(int(duration*1000))]

    # contrary to c++, in python it is interesting to build arrays
    # with connected motors indexes so we can simply go through them in main loop
    motors_spi_connected_indexes = [] # indexes of the motors on each connected slaves

    print("-- Start of example script --")

    os.nice(-20)  #  Set the process to highest priority (from -20 highest to +20 lowest)
    robot_if = mbs.MasterBoardInterface(name_interface)
    robot_if.Init()  # Initialization of the interface between the computer and the master board
    for i in range(N_SLAVES_CONTROLED):  #  We enable each controler driver and its two associated motors
        robot_if.GetDriver(i).motor1.SetCurrentReference(0)
        robot_if.GetDriver(i).motor2.SetCurrentReference(0)
        robot_if.GetDriver(i).motor1.Enable()
        robot_if.GetDriver(i).motor2.Enable()
        robot_if.GetDriver(i).EnablePositionRolloverError()
        robot_if.GetDriver(i).SetTimeout(5)
        robot_if.GetDriver(i).Enable()

    last = clock()

    while (not robot_if.IsTimeout() and not robot_if.IsAckMsgReceived()):
        if ((clock() - last) > dt):
            last = clock()
            robot_if.SendInit()

    if robot_if.IsTimeout():
        print("Timeout while waiting for ack.")
    else:

        # fill the connected motors indexes array
        for i in range(N_SLAVES_CONTROLED):
            if robot_if.GetDriver(i).is_connected:
                # if slave i is connected then motors 2i and 2i+1 are potentially connected
                motors_spi_connected_indexes.append(2 * i)
                motors_spi_connected_indexes.append(2 * i + 1)

    while ((not robot_if.IsTimeout())
           and (clock() < duration)):  # Stop after 15 seconds (around 5 seconds are used at the start for calibration)

        if(received_list[robot_if.GetLastRecvCmdIndex()] == 0) :
                received_list[robot_if.GetLastRecvCmdIndex()] = clock()
        
        if ((clock() - last) > dt):
            last = clock()
            cpt += 1
            
            robot_if.ParseSensorData()  # Read sensor data sent by the masterboard

            if (state == 0):  #  If the system is not ready
                state = 1

                # for all motors on a connected slave
                for i in motors_spi_connected_indexes:  # Check if all motors are enabled and ready
                    if not (robot_if.GetMotor(i).IsEnabled() and robot_if.GetMotor(i).IsReady()):
                        state = 0

            else:  # If the system is ready

                # for all motors on a connected slave
                for i in motors_spi_connected_indexes:

                    if i % 2 == 0 and robot_if.GetDriver(i // 2).error_code == 0xf:
                        #print("Transaction with SPI{} failed".format(i // 2))
                        continue #user should decide what to do in that case, here we ignore that motor

                    if robot_if.GetMotor(i).IsEnabled():
                        robot_if.GetMotor(i).SetCurrentReference(0)  # Set reference currents

            if ((cpt % 100) == 0):  # Display state of the system once every 100 iterations of the main loop
                print(chr(27) + "[2J")
                # To read IMU data in Python use robot_if.imu_data_accelerometer(i), robot_if.imu_data_gyroscope(i)
                # or robot_if.imu_data_attitude(i) with i = 0, 1 or 2
                robot_if.PrintCmdStats()
                robot_if.PrintSensorStats()
                sys.stdout.flush()  # for Python 2, use print( .... , flush=True) for Python 3


            sent_list[robot_if.GetCmdPacketIndex()] = clock()
            robot_if.SendCommand()  # Send the reference currents to the master board
            

    robot_if.Stop()  # Shut down the interface between the computer and the master board


    latency = []
    for i in range(1, len(sent_list)-1):
        if(received_list[i] != 0 and sent_list[i] != 0) :
            latency.append(received_list[i]-sent_list[i])

    if latency != [] :
        average = sum(latency)/len(latency)
        print("average latency : %f s" %average)

        plt.plot(range(len(latency)), latency, 'ro')
        plt.ylabel('latency (s)')
        plt.text(0, max(latency), 'average latency : %f s' %average)
        plt.show()

    if robot_if.IsTimeout():
        print("Masterboard timeout detected.")
        print("Either the masterboard has been shut down or there has been a connection issue with the cable/wifi.")

    print("-- End of example script --")


def main():
    parser = argparse.ArgumentParser(description='Example masterboard use in python.')
    parser.add_argument('-i',
                        '--interface',
                        required=True,
                        help='Name of the interface (use ifconfig in a terminal), for instance "enp1s0"')

    example_script(parser.parse_args().interface)


if __name__ == "__main__":
    main()
