# coding: utf8

import argparse
import os
import sys
import time
import subprocess
try:
  from time import perf_counter
except(ImportError):
  # You are still in python2… pleas upgrade :)
  from time import clock as perf_counter


import libmaster_board_sdk_pywrap as mbs
import matplotlib.pyplot as plt
from matplotlib.offsetbox import AnchoredText
import numpy as np

linux_distribution = None
if not linux_distribution:
    try:
        from platform import linux_distribution
    except(ImportError):
        pass
if not linux_distribution:
    try:
        from distro import linux_distribution
    except(ImportError):
        pass


def example_script(name_interface):

    N_SLAVES = 6  #  Maximum number of controled drivers
    N_SLAVES_CONTROLED = 6  # Current number of controled drivers

    cpt = 0
    dt = 0.001  #  Time step
    state = 0  # State of the system (ready (1) or not (0))
    duration = 30 # Duration in seconds, init included
    
    cmd_lost_list = []
    sensor_lost_list = []
    cmd_ratio_list = []
    sensor_ratio_list = []
    time_list = []
    histogram_sensor = []
    histogram_cmd = []

    if (duration == 0):
        print("Null duration selected, end of script")
        return

    sent_list = [0.0 for i in range(int(duration/dt)+2)]
    received_list = [0.0 for i in range(int(duration/dt)+2)]
    loop_duration = []

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

    last = perf_counter()
    prev_time = 0

    while (not robot_if.IsTimeout() and not robot_if.IsAckMsgReceived()):
        if ((perf_counter() - last) > dt):
            last = perf_counter()
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
    
    overflow_cmd_cpt = 0
    first_cmd_index = robot_if.GetCmdPacketIndex()
    cmd_packet_index = first_cmd_index
    last_cmd_packet_index = first_cmd_index

    while ((not robot_if.IsTimeout())
           and (perf_counter() < duration)):  # Stop after 15 seconds (around 5 seconds are used at the start for calibration)

        if (robot_if.GetLastRecvCmdIndex() > robot_if.GetCmdPacketIndex()):
            last_recv_cmd_index = (overflow_cmd_cpt-1) * 65536 + robot_if.GetLastRecvCmdIndex()
        else:
            last_recv_cmd_index = overflow_cmd_cpt * 65536 + robot_if.GetLastRecvCmdIndex()

        if (last_recv_cmd_index >= first_cmd_index and received_list[last_recv_cmd_index-first_cmd_index] == 0):
                received_list[last_recv_cmd_index-first_cmd_index] = perf_counter()
        
        if ((perf_counter() - last) > dt):
            last += dt
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
                robot_if.PrintStats()
                sys.stdout.flush()  # for Python 2, use print( .... , flush=True) for Python 3

                cmd_lost_list.append(robot_if.GetCmdLost())
                sensor_lost_list.append(robot_if.GetSensorsLost())
                cmd_ratio_list.append(100.*robot_if.GetCmdLost()/robot_if.GetCmdSent())
                sensor_ratio_list.append(100.*robot_if.GetSensorsLost()/robot_if.GetSensorsSent())
                time_list.append(perf_counter())

            current_time = perf_counter()

            diff = robot_if.GetCmdPacketIndex() - last_cmd_packet_index
            if diff < 0:
                overflow_cmd_cpt += 1
                diff = 65536 + robot_if.GetCmdPacketIndex() - last_cmd_packet_index
            cmd_packet_index += diff 
            last_cmd_packet_index = robot_if.GetCmdPacketIndex()

            robot_if.SendCommand()  # Send the reference currents to the master board
            
            sent_list[cmd_packet_index-first_cmd_index] = current_time
            if (prev_time != 0) :
                loop_duration.append(1000 * (current_time - prev_time))
            prev_time = current_time
        
    robot_if.Stop()  # Shut down the interface between the computer and the master board

    if robot_if.IsTimeout():
        print("Masterboard timeout detected.")
        print("Either the masterboard has been shut down or there has been a connection issue with the cable/wifi.")
        if cpt == 0:
            print("-- End of example script --")
            return


    # creation of the folder where the graphs will be stored
    if not os.path.isdir("../graphs"):
        os.mkdir("../graphs")
        
    dir_name = time.strftime("%m") + time.strftime("%d") + time.strftime("%H") + time.strftime("%M") + time.strftime("%S")
    if os.path.isdir("../graphs/" + dir_name):
        count = 1
        while os.path.isdir("../graphs/" + dir_name + "-" + str(count)):
            count += 1
        dir_name += "-" + str(count)
        
    
    os.mkdir("../graphs/" + dir_name)


    latency = []
    for i in range(1, len(sent_list)-1):
        if (received_list[i] != 0 and sent_list[i] != 0):
            latency.append(1000 * (received_list[i] - sent_list[i]))
        else:
            latency.append(0) # 0 means not sent or not received

    # computing avg and std for non zero values
    nonzero = [latency[i] for i in np.nonzero(latency)[0]]
    average = 0
    std = 0
    if len(nonzero) != 0:
        average = np.mean(nonzero)
        print("average latency: %f ms" %average)
        std = np.std(nonzero)
        print("standard deviation: %f ms" %std)


    plt.figure("wifi-ethernet latency", figsize=(20,15), dpi=200)

    anchored_text = AnchoredText("average latency: %f ms\nstandard deviation: %f ms" %(average, std), loc=2, prop=dict(fontsize='xx-large'))

    if len(latency) > 5000:
        ax1 = plt.subplot(2, 1, 1)
    else:
        ax1 = plt.subplot(1, 1, 1)
    ax1.plot(latency, '.')
    ax1.set_xlabel("index", fontsize='xx-large')
    ax1.set_ylabel("latency (ms)", fontsize='xx-large')
    ax1.add_artist(anchored_text)

    # plotting zoomed version to see pattern
    if len(latency) > 5000:
        ax2 = plt.subplot(2, 1, 2)
        ax2.plot(latency, '.')
        ax2.set_xlabel("index", fontsize='xx-large')
        ax2.set_ylabel("latency (ms)", fontsize='xx-large')
        ax2.set_xlim(len(latency)/2, len(latency)/2 + 2000)
        ax2.set_ylim(-0.1, 2.1)

    if (name_interface[0] == 'w'):
        freq = subprocess.check_output("iwlist " + name_interface +" channel | grep Frequency", shell = True)
        channel = (str(freq).split('(')[1]).split(')')[0]
        plt.suptitle("Wifi communication latency: " + channel, fontsize='xx-large')

    else :
        plt.suptitle("Ethernet communication latency", fontsize='xx-large')
    

    plt.savefig("../graphs/" + dir_name + '/' + dir_name + "-wifieth-latency.png")


    #Plot histograms and graphs
    for i in range(20):
        histogram_sensor.append(robot_if.GetSensorHistogram(i))
        histogram_cmd.append(robot_if.GetCmdHistogram(i))
    
    plt.figure("wifi-ethernet statistics", figsize=(20,15), dpi=200)
    
    ax3 = plt.subplot(3, 2, 1)
    ax3.bar(range(1, 21), histogram_cmd)
    ax3.set_xlabel("Number of consecutive lost command packets", fontsize='xx-large')
    ax3.set_ylabel("Number of occurences", fontsize='xx-large')
    ax3.set_xticks(range(1, 21))

    ax4 = plt.subplot(3, 2, 2)
    ax4.bar(range(1, 21), histogram_sensor)
    ax4.set_xlabel("Number of consecutive lost sensor packets", fontsize='xx-large')
    ax4.set_ylabel("Number of occurences", fontsize='xx-large') 
    ax4.set_xticks(range(1, 21))

    ax5 = plt.subplot(3, 2, 3)
    ax5.plot(time_list, cmd_lost_list)
    ax5.set_xlabel("time (s)", fontsize='xx-large')
    ax5.set_ylabel("lost commands", fontsize='xx-large')

    ax6 = plt.subplot(3, 2, 4)
    ax6.plot(time_list, sensor_lost_list)
    ax6.set_xlabel("time (s)", fontsize='xx-large')
    ax6.set_ylabel("lost sensors", fontsize='xx-large')

    ax7 = plt.subplot(3, 2, 5)
    ax7.plot(time_list, cmd_ratio_list)
    ax7.set_xlabel("time (s)", fontsize='xx-large')
    ax7.set_ylabel("ratio command loss (%)", fontsize='xx-large')

    ax8 = plt.subplot(3, 2, 6)
    ax8.plot(time_list, sensor_ratio_list)
    ax8.set_xlabel("time (s)", fontsize='xx-large')
    ax8.set_ylabel("ratio sensor loss (%)", fontsize='xx-large')

    if (name_interface[0] == 'w'):
        plt.suptitle("Wifi statistics: " + channel, fontsize='xx-large')

    else :
        plt.suptitle("Ethernet statistics", fontsize='xx-large')


    plt.savefig("../graphs/" + dir_name + '/' + dir_name + "-wifieth-stats.png")


    # plotting the evolution of command loop duration
    plt.figure("loop duration", figsize=(20, 15), dpi=200)
    anchored_text = AnchoredText("average duration: %f ms\nstandard deviation: %f ms" %(np.mean(loop_duration), np.std(loop_duration)), loc=2, prop=dict(fontsize='xx-large'))

    ax9 = plt.subplot(1, 1, 1)
    ax9.plot(loop_duration, '.')
    ax9.set_xlabel("iteration", fontsize='xx-large')
    ax9.set_ylabel("loop duration (ms)", fontsize='xx-large')
    ax9.add_artist(anchored_text)

    plt.suptitle("Command loop duration", fontsize='xx-large')

    plt.savefig("../graphs/" + dir_name + '/' + dir_name + "-command-loop-duration.png")


    # creation of the text file with system info
    text_file = open("../graphs/" + dir_name + '/' + dir_name + "-system-info.txt", 'w')
    text_file.write("Current date and time: " + time.strftime("%c") + '\n')
    if(linux_distribution):
        text_file.write("Linux distribution: " + linux_distribution()[0] + ' ' + linux_distribution()[1] + ' ' + linux_distribution()[2] + '\n')
    else:
        text_file.write("Linux distribution: ? (`plateform` or `distro` module necessary for logging this info)\n")
    text_file.write("Computer: " + os.uname()[1] + '\n')
    text_file.write("Interface: " + name_interface + '\n')
    if (name_interface[0] == 'w'):
        text_file.write("Wifi channel: " + channel + '\n')
    text_file.write("Protocol version: " + str(robot_if.GetProtocolVersion()) + '\n')
    text_file.write("Script duration: " + str(duration) + '\n')
    text_file.close()

    print("The graphs have been stored in 'master_board_sdk/graphs' with a text file containing the system info, in a folder named from the current date and time.")
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
