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


def example_script(name_interface):

    N_SLAVES = 6  #  Maximum number of controled drivers
    N_SLAVES_CONTROLED = 6  # Current number of controled drivers

    cpt = 0  # Iteration counter
    dt = 0.001  #  Time step
    t = 0  # Current time
    kp = 5.0  #  Proportional gain
    kd = 0.1  # Derivative gain
    iq_sat = 1.0  # Maximum amperage (A)
    freq = 0.5  # Frequency of the sine wave
    amplitude = math.pi  # Amplitude of the sine wave
    init_pos = [0.0 for i in range(N_SLAVES * 2)]  # List that will store the initial position of motors
    state = 0  # State of the system (ready (1) or not (0))

    # contrary to c++, in python it is interesting to build arrays
    # with connected motors indexes so we can simply go through them in main loop
    motors_spi_connected_indexes = [] # indexes of the motors on each connected slaves

    print("-- Start of example script --")

    os.nice(-20)  #  Set the process to highest priority (from -20 highest to +20 lowest)
    robot_if = mbs.MasterBoardInterface(name_interface)
    robot_if.Init()  # Initialization of the interface between the computer and the master board
    for i in range(N_SLAVES_CONTROLED):  #  We enable each controler driver and its two associated motors
        # initialise command to zero
        robot_if.GetDriver(i).motor1.SetCurrentReference(0)
        robot_if.GetDriver(i).motor1.SetPositionReference(0)
        robot_if.GetDriver(i).motor1.SetVelocityReference(0)
        robot_if.GetDriver(i).motor1.SetKp(0)
        robot_if.GetDriver(i).motor1.SetKd(0)
        robot_if.GetDriver(i).motor2.SetCurrentReference(0)
        robot_if.GetDriver(i).motor2.SetPositionReference(0)
        robot_if.GetDriver(i).motor2.SetVelocityReference(0)
        robot_if.GetDriver(i).motor2.SetKp(0)
        robot_if.GetDriver(i).motor2.SetKd(0)

        robot_if.GetDriver(i).motor1.Enable()
        robot_if.GetDriver(i).motor2.Enable()
        robot_if.GetDriver(i).EnablePositionRolloverError()
        robot_if.GetDriver(i).SetTimeout(5)
        robot_if.GetDriver(i).Enable()

    last = perf_counter()
    print("last =",last)
    while (not robot_if.IsTimeout() and not robot_if.IsAckMsgReceived()):
        if ((perf_counter() - last) > dt):
            last = perf_counter()
            robot_if.SendInit()

    if robot_if.IsTimeout():
        print("Timeout while waiting for ack.")
    else:

        # fill the connected motors indexes array
        for i in range(N_SLAVES_CONTROLED):
            if robot_if.GetDriver(i).IsConnected():
                # if slave i is connected then motors 2i and 2i+1 are potentially connected
                motors_spi_connected_indexes.append(2 * i)
                motors_spi_connected_indexes.append(2 * i + 1)
    glob_last = perf_counter()

    # Stop after 15 seconds (around 5 seconds are used at the start for calibration)
    while ((not robot_if.IsTimeout())
           and ( (perf_counter() - glob_last) < 20)):  # Stop after 15 seconds (around 5 seconds are used at the start for calibration)

        if ((perf_counter() - last) > dt):
            last = perf_counter()
            cpt += 1
            t += dt
            robot_if.ParseSensorData()  # Read sensor data sent by the masterboard

            if (state == 0):  #  If the system is not ready
                state = 1

                # for all motors on a connected slave
                for i in motors_spi_connected_indexes:  # Check if all motors are enabled and ready
                    if not (robot_if.GetMotor(i).IsEnabled() and robot_if.GetMotor(i).IsReady()):
                        state = 0
                    init_pos[i] = robot_if.GetMotor(i).GetPosition()
                    t = 0

            else:  # If the system is ready

                # for all motors on a connected slave
                for i in motors_spi_connected_indexes:

                    if i % 2 == 0 and robot_if.GetDriver(i // 2).GetErrorCode() == 0xf:
                        #print("Transaction with SPI{} failed".format(i // 2))
                        continue #user should decide what to do in that case, here we ignore that motor

                    if robot_if.GetMotor(i).IsEnabled():
                        print("sending ",i," motor settings.")
                        ref = init_pos[i] + amplitude * math.sin(2.0 * math.pi * freq * t)  # Sine wave pattern
                        v_ref = 2.0 * math.pi * freq * amplitude * math.cos(2.0 * math.pi * freq * t)
                        p_err = ref - robot_if.GetMotor(i).GetPosition()  # Position error
                        v_err = v_ref - robot_if.GetMotor(i).GetVelocity()  # Velocity error
                        cur = kp * p_err + kd * v_err  #  Output of the PD controler (amperage)
                        if (cur > iq_sat):  #  Check saturation
                            cur = iq_sat
                        if (cur < -iq_sat):
                            cur = -iq_sat
                        robot_if.GetMotor(i).SetCurrentReference(cur)  # Set reference currents

            if ((cpt % 100) == 0):  # Display state of the system once every 100 iterations of the main loop
                print(chr(27) + "[2J")
                # To read IMU data in Python use robot_if.imu_data_accelerometer(i), robot_if.imu_data_gyroscope(i)
                # or robot_if.imu_data_attitude(i) with i = 0, 1 or 2
                robot_if.PrintIMU()
                robot_if.PrintPowerBoard()
                robot_if.PrintADC()
                robot_if.PrintMotors()
                robot_if.PrintMotorDrivers()
                robot_if.PrintStats()
                sys.stdout.flush()  # for Python 2, use print( .... , flush=True) for Python 3

            robot_if.SendCommand()  # Send the reference currents to the master board

    robot_if.Stop()  # Shut down the interface between the computer and the master board

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
