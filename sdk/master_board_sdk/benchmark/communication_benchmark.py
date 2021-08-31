# coding: utf8

from pathlib import Path
import argparse
import math
import os
import sys
import time
from copy import deepcopy
from time import clock
import yaml

import libmaster_board_sdk_pywrap as mbs
import matplotlib.pyplot as plt
from matplotlib.offsetbox import AnchoredText
import numpy as np
import platform
import subprocess


class LatencyEstimator(object):
    def __init__(self, duration, dt, last_recv_cmd_index):
        super().__init__()

        self._overflow_cmd_cpt = 0
        self._first_cmd_index = last_recv_cmd_index
        self._cmd_packet_index = last_recv_cmd_index
        self._last_cmd_packet_index = last_recv_cmd_index

        self._sent_list = (int(duration / dt) + 2) * [0.0]
        self._received_list = (int(duration / dt) + 2) * [0.0]

        self._latency = []

    def update_received_list(self, last_recv_cmd_index, current_cmd_packet_index):
        if last_recv_cmd_index > current_cmd_packet_index:
            last_recv_cmd_index = (
                self._overflow_cmd_cpt - 1
            ) * 65536 + last_recv_cmd_index
        else:
            last_recv_cmd_index = self._overflow_cmd_cpt * 65536 + last_recv_cmd_index

        if (
            last_recv_cmd_index >= self._first_cmd_index
            and self._received_list[last_recv_cmd_index - self._first_cmd_index] == 0
        ):
            self._received_list[last_recv_cmd_index - self._first_cmd_index] = clock()

    def update_sent_list(self, current_cmd_packet_index):
        diff = current_cmd_packet_index - self._last_cmd_packet_index
        if diff < 0:
            self._overflow_cmd_cpt += 1
            diff = 65536 + current_cmd_packet_index - self._last_cmd_packet_index
        self._cmd_packet_index += diff
        self._last_cmd_packet_index = current_cmd_packet_index
        self._sent_list[self._cmd_packet_index - self._first_cmd_index] = clock()

    def get_latency_measurement(self):
        self._latency = []
        for i in range(1, len(self._sent_list) - 1):
            if self._received_list[i] != 0 and self._sent_list[i] != 0:
                self._latency.append(
                    1000 * (self._received_list[i] - self._sent_list[i])
                )
            else:
                self._latency.append(0)  # 0 means not sent or not received
        return self._latency


class DurationEstimator(object):
    def __init__(self) -> None:
        super().__init__()
        self._previous_time = 0.0
        self._current_time = 0.0

    def initialize(self):
        self._previous_time = clock()

    def get_duration(self):
        self._current_time = clock()
        if self._previous_time != 0.0:
            return 1000 * (self._current_time - self._previous_time)
        self._previous_time = self._current_time

    def get_current_time(self):
        return 1000 * self._current_time


class BenchmarkData(object):
    def __init__(self):

        # Data storage.

        # 100Hz data buffer
        self.time_list = []
        self.cmd_lost_list = []
        self.sensor_lost_list = []
        self.cmd_ratio_list = []
        self.sensor_ratio_list = []

        # Histograms
        self.histogram_sensor = []
        self.histogram_cmd = []

        # Communication latency
        self.latency = []

        # Control time loop
        self.loop_duration = []

        # Data folder to dump the data/figures.
        self._data_folder = (
            Path("/tmp")
            / "communication_benchmark"
            / deepcopy(time.strftime("%Y-%m-%d_%H-%M-%S"))
        )
        if not self._data_folder.is_dir():
            self._data_folder.mkdir(parents=True, exist_ok=True)

        # Metadata and statistics.
        self.metadata = {}
        self.metadata["controller"] = {}
        self.metadata["system"] = {}
        self.metadata["latency_statistics"] = {}

    def initialize(self, dt, network_interface, protocol_version):
        # collect infos.
        self.dt = dt
        self.network_interface = network_interface

        # get the wifi channel
        if self.network_interface[0] == "w":
            freq = subprocess.check_output(
                "iwlist " + self.network_interface + " channel | grep Frequency",
                shell=True,
            )
            self.wifi_channel = (str(freq).split("(")[1]).split(")")[0]
        else:
            self.wifi_channel = "None"

        # Controller infos.
        self.metadata["controller"]["control_period"] = self.dt
        self.metadata["controller"]["network_interface"] = self.network_interface
        self.metadata["controller"]["wifi_channel"] = self.wifi_channel

        # System infos.
        self.metadata["system"]["date_time"] = time.strftime("%c")
        self.metadata["system"]["linux_distribution"] = (
            platform.linux_distribution()[0]
            + " "
            + platform.linux_distribution()[1]
            + " "
            + platform.linux_distribution()[2]
        )
        self.metadata["system"]["computer"] = os.uname()[1]
        self.metadata["system"]["protocole_version"] = protocol_version

    def dump_data(self):

        self._dump_latency()
        self._dump_histograms_and_graphs()
        self._dump_loop_duration()
        self._dump_metadata()
        print(
            "The graphs and metadata have been stored in ",
            self._data_folder,
        )

    def _dump_metadata(self):
        """Dump the metada in a yaml format to be human readable."""
        yaml_text = yaml.dump(self.metadata, default_flow_style=False)
        with open(str(self._data_folder / "metadata.yaml"), "a") as f:
            f.write(yaml_text)
            f.close()

    def _dump_latency(self):
        # computing avg and std for non zero values
        nonzero = [self.latency[i] for i in np.nonzero(self.latency)[0]]
        average = 0
        std = 0
        if len(nonzero) != 0:
            average = np.mean(nonzero)
            std = np.std(nonzero)
            self.metadata["latency_statistics"]["average"] = average
            self.metadata["latency_statistics"]["standard_deviation"] = std
            print("latency average latency: %f ms" % average)
            print("latency standard deviation: %f ms" % std)

        print(print("-- Dump latency graph --"))
        plt.figure("wifi-ethernet latency", figsize=(20, 15), dpi=200)
        anchored_text = AnchoredText(
            "average latency: %f ms\nstandard deviation: %f ms" % (average, std),
            loc=2,
            prop=dict(fontsize="xx-large"),
        )
        if len(self.latency) > 5000:
            ax1 = plt.subplot(2, 1, 1)
        else:
            ax1 = plt.subplot(1, 1, 1)
        ax1.plot(self.latency, ".")
        ax1.set_xlabel("index", fontsize="xx-large")
        ax1.set_ylabel("latency (ms)", fontsize="xx-large")
        ax1.add_artist(anchored_text)

        # plotting zoomed version to see pattern
        if len(self.latency) > 5000:
            ax2 = plt.subplot(2, 1, 2)
            ax2.plot(self.latency, ".")
            ax2.set_xlabel("index", fontsize="xx-large")
            ax2.set_ylabel("latency (ms)", fontsize="xx-large")
            ax2.set_xlim(len(self.latency) / 2, len(self.latency) / 2 + 2000)
            ax2.set_ylim(-0.1, 2.1)

        if self.wifi_channel != "None":
            plt.suptitle(
                "Wifi communication latency: " + self.wifi_channel,
                fontsize="xx-large",
            )
        else:
            plt.suptitle("Ethernet communication latency", fontsize="xx-large")
        plt.savefig(str(self.data_folder / "wifi_eth_latency.png"))

    def _dump_histograms_and_graphs(self):
        # Plot histograms and graphs

        plt.figure("wifi-ethernet statistics", figsize=(20, 15), dpi=200)

        ax3 = plt.subplot(3, 2, 1)
        ax3.bar(range(1, 21), self.histogram_cmd)
        ax3.set_xlabel(
            "Number of consecutive lost command packets", fontsize="xx-large"
        )
        ax3.set_ylabel("Number of occurences", fontsize="xx-large")
        ax3.set_xticks(range(1, 21))

        ax4 = plt.subplot(3, 2, 2)
        ax4.bar(range(1, 21), self.histogram_sensor)
        ax4.set_xlabel("Number of consecutive lost sensor packets", fontsize="xx-large")
        ax4.set_ylabel("Number of occurences", fontsize="xx-large")
        ax4.set_xticks(range(1, 21))

        ax5 = plt.subplot(3, 2, 3)
        ax5.plot(self.time_list, self.cmd_lost_list)
        ax5.set_xlabel("time (s)", fontsize="xx-large")
        ax5.set_ylabel("lost commands", fontsize="xx-large")

        ax6 = plt.subplot(3, 2, 4)
        ax6.plot(self.time_list, self.sensor_lost_list)
        ax6.set_xlabel("time (s)", fontsize="xx-large")
        ax6.set_ylabel("lost sensors", fontsize="xx-large")

        ax7 = plt.subplot(3, 2, 5)
        ax7.plot(self.time_list, self.cmd_ratio_list)
        ax7.set_xlabel("time (s)", fontsize="xx-large")
        ax7.set_ylabel("ratio command loss (%)", fontsize="xx-large")

        ax8 = plt.subplot(3, 2, 6)
        ax8.plot(self.time_list, self.sensor_ratio_list)
        ax8.set_xlabel("time (s)", fontsize="xx-large")
        ax8.set_ylabel("ratio sensor loss (%)", fontsize="xx-large")

        if self.wifi_channel != "None":
            plt.suptitle("Wifi statistics: " + self.wifi_channel, fontsize="xx-large")

        else:
            plt.suptitle("Ethernet statistics", fontsize="xx-large")
        plt.savefig(str(self.data_folder / "wifi_eth_stats.png"))

    def _dump_loop_duration(self):
        """plotting the evolution of command loop duration"""

        plt.figure("loop duration", figsize=(20, 15), dpi=200)
        anchored_text = AnchoredText(
            "average duration: %f ms\nstandard deviation: %f ms"
            % (np.mean(self.loop_duration), np.std(self.loop_duration)),
            loc=2,
            prop=dict(fontsize="xx-large"),
        )
        ax9 = plt.subplot(1, 1, 1)
        ax9.plot(self.loop_duration, ".")
        ax9.set_xlabel("iteration", fontsize="xx-large")
        ax9.set_ylabel("loop duration (ms)", fontsize="xx-large")
        ax9.add_artist(anchored_text)
        plt.suptitle("Command loop duration", fontsize="xx-large")
        plt.savefig(str(self.data_folder / "command_loop_duration.png"))


def communication_benchmark(name_interface):
    # input paramters
    n_udriver_controlled = 1  # Current number of controled drivers
    cpt = 0  # Counting iterations.
    dt = 0.001  #  Time step
    state = 0  # State of the system (ready (1) or not (0))
    duration = 20  # Duration in seconds, init included

    # Check input parameters
    if duration == 0:
        print("-- Null duration selected, end of script --")
        return
    print("-- Start of example script --")

    #  Set the process to highest priority (from -20 highest to +20 lowest)
    os.nice(-20)

    # Create the communication interface to the robot.
    robot_if = mbs.MasterBoardInterface(name_interface)
    # Initialization of the interface between the computer and the master board
    robot_if.Init()
    #  We enable each controler driver and its two associated motors
    for i in range(n_udriver_controlled):
        robot_if.GetDriver(i).motor1.SetCurrentReference(0)
        robot_if.GetDriver(i).motor2.SetCurrentReference(0)
        robot_if.GetDriver(i).motor1.Enable()
        robot_if.GetDriver(i).motor2.Enable()
        robot_if.GetDriver(i).EnablePositionRolloverError()
        robot_if.GetDriver(i).SetTimeout(5)
        robot_if.GetDriver(i).Enable()

    # Start sending the "get ready command"
    last = clock()
    while not robot_if.IsTimeout() and not robot_if.IsAckMsgReceived():
        if (clock() - last) > dt:
            last = clock()
            robot_if.SendInit()

    # contrary to c++, in python it is interesting to build arrays
    # with connected motors indexes so we can simply go through them in main loop
    # indexes of the motors on each connected slaves
    motors_spi_connected_indexes = []
    # Check if the interface is properly connected.
    if robot_if.IsTimeout():
        print("-- Timeout while waiting for ack. --")
    else:
        # fill the connected motors indexes array
        for i in range(n_udriver_controlled):
            if robot_if.GetDriver(i).is_connected:
                # if slave i is connected then motors 2i and 2i+1 are potentially connected
                motors_spi_connected_indexes.append(2 * i)
                motors_spi_connected_indexes.append(2 * i + 1)

    latency_estimator = LatencyEstimator(duration, dt, robot_if.GetCmdPacketIndex())
    loop_duration_estimator = DurationEstimator()
    benchmark_data = BenchmarkData()

    benchmark_data.initialize(dt, name_interface, str(robot_if.GetProtocolVersion()))
    loop_duration_estimator.initialize()
    while (not robot_if.IsTimeout()) and (
        clock() < duration
    ):  # Stop after 15 seconds (around 5 seconds are used at the start for calibration)

        latency_estimator.update_received_list(
            robot_if.GetLastRecvCmdIndex(), robot_if.GetCmdPacketIndex()
        )

        # Fix the control period.
        if (clock() - last) > dt:
            last += dt
            cpt += 1

            # Read sensor data sent by the masterboard
            robot_if.ParseSensorData()

            if state == 0:  #  If the system is not ready
                state = 1
                # for all motors on a connected slave
                # Check if all motors are enabled and ready
                for i in motors_spi_connected_indexes:
                    if not (
                        robot_if.GetMotor(i).IsEnabled()
                        and robot_if.GetMotor(i).IsReady()
                    ):
                        state = 0
            # If the system is ready
            else:

                # for all motors on a connected slave
                for i in motors_spi_connected_indexes:
                    if i % 2 == 0 and robot_if.GetDriver(i // 2).error_code == 0xF:
                        # print("Transaction with SPI{} failed".format(i // 2))
                        continue  # user should decide what to do in that case, here we ignore that motor
                    if robot_if.GetMotor(i).IsEnabled():
                        # Set reference currents, TODO create a proper controller here.
                        robot_if.GetMotor(i).SetCurrentReference(0)

            # Display system state and communication status,
            # once every 100 iterations of the main loop
            if cpt % 100 == 0:
                print(chr(27) + "[2J")
                # To read IMU data in Python use robot_if.imu_data_accelerometer(i), robot_if.imu_data_gyroscope(i)
                # or robot_if.imu_data_attitude(i) with i = 0, 1 or 2
                robot_if.PrintStats()
                sys.stdout.flush()  # for Python 2, use print( .... , flush=True) for Python 3

                benchmark_data.cmd_lost_list.append(robot_if.GetCmdLost())
                benchmark_data.sensor_lost_list.append(robot_if.GetSensorsLost())
                benchmark_data.cmd_ratio_list.append(
                    100.0 * robot_if.GetCmdLost() / robot_if.GetCmdSent()
                )
                benchmark_data.sensor_ratio_list.append(
                    100.0 * robot_if.GetSensorsLost() / robot_if.GetSensorsSent()
                )
                benchmark_data.time_list.append(clock())

            latency_estimator.update_sent_list(robot_if.GetCmdPacketIndex())
            robot_if.SendCommand()  # Send the reference currents to the master board
            benchmark_data.loop_duration.append(loop_duration_estimator.get_duration())

    robot_if.Stop()  # Shut down the interface between the computer and the master board
    if robot_if.IsTimeout():
        print("-- Masterboard timeout detected. --")
        print(
            "-- ",
            "Either the masterboard has been shutdown, ",
            "or there has been a connection issue with the cable/wifi." " --",
        )

    benchmark_data.latency = latency_estimator.get_latency_measurement()
    # Plot histograms and graphs
    for i in range(20):
        benchmark_data.histogram_sensor.append(robot_if.GetSensorHistogram(i))
        benchmark_data.histogram_cmd.append(robot_if.GetCmdHistogram(i))

    print("-- Dump the collected data --")
    benchmark_data.dump_data()

    print("-- End of example script --")


def main():
    parser = argparse.ArgumentParser(description="Example masterboard use in python.")
    parser.add_argument(
        "-i",
        "--interface",
        required=True,
        help='Name of the interface (use ifconfig in a terminal), for instance "enp1s0"',
    )

    communication_benchmark(parser.parse_args().interface)


if __name__ == "__main__":
    main()
