# coding: utf8

from time import clock
import math
import sys
import getopt
import os
import libmaster_board_sdk_pywrap as mbs


def example_script(name_interface):

    N_SLAVES = 6  #  Maximum number of controled drivers
    N_SLAVES_CONTROLED = 2  # Current number of controled drivers

    cpt = 0  # Iteration counter
    dt = 0.001  #  Time step
    t = 0  # Current time
    kp = 5.0  #  Proportional gain
    kd = 0.5  # Derivative gain
    iq_sat = 1.0  # Maximum amperage (A)
    freq = 0.5  # Frequency of the sine wave
    amplitude = math.pi  # Amplitude of the sine wave
    init_pos = [0.0 for i in range(N_SLAVES * 2)]  # List that will store the initial position of motors
    state = 0  # State of the system (ready (1) or not (0))

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

    while (clock() < 20):  # Stop after 15 seconds (around 5 seconds are used at the start for calibration)

        if ((clock() - last) > dt):
            last = clock()
            cpt += 1
            t += dt
            robot_if.ParseSensorData()  # Read sensor data sent by the masterboard

            if (state == 0):  #  If the system is not ready
                state = 1
                for i in range(N_SLAVES_CONTROLED * 2):  # Check if all motors are enabled and ready
                    if not (robot_if.GetMotor(i).IsEnabled() and robot_if.GetMotor(i).IsReady()):
                        state = 0
                    init_pos[i] = robot_if.GetMotor(i).GetPosition()
                    t = 0
            else:  # If the system is ready
                for i in range(N_SLAVES_CONTROLED * 2):
                    if robot_if.GetMotor(i).IsEnabled():
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
                robot_if.PrintIMU()
                robot_if.PrintADC()
                robot_if.PrintMotors()
                robot_if.PrintMotorDrivers()
                sys.stdout.flush()  # for Python 2, use print( .... , flush=True) for Python 3

            robot_if.SendCommand()  # Send the reference currents to the master board

    robot_if.Stop()  # Shut down the interface between the computer and the master board

    print("-- End of example script --")


def main(argv):
    name_interface = ""  # Name of the interface (use ifconfig in a terminal), for instance "enp1s0"
    try:
        opts, args = getopt.getopt(argv, "hi:")
    except getopt.GetoptError:
        print("example.py -i <interface>")
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print("example.py -i <interface>")
            sys.exit()
        elif opt in ("-i") and isinstance(arg, str):
            name_interface = arg
    print("name_interface: ", name_interface)
    example_script(name_interface)


if __name__ == "__main__":
    main(sys.argv[1:])
