#! /usr/bin/env python3
"""
BSD 3-Clause License

Copyright (c) 2019, LAAS-CNRS, New York University and Max Planck
Gesellschaft.
All rights reserved.
"""
import argparse
from pathlib import Path
import pandas
import yaml
import pyaml
import numpy as np
from matplotlib.offsetbox import AnchoredText
from matplotlib import pyplot as plt


def main(path_to_data_folder_in):
    path_to_data_folder = Path(path_to_data_folder_in)
    assert path_to_data_folder.exists()

    # display the metadata
    print("Metada:")
    print("---")
    with open(path_to_data_folder / "meta_data.yaml") as yaml_file:
        metadata = yaml.safe_load(yaml_file)
        print(pyaml.dump(metadata))
    print("---")
    print()

    # Read the data
    data = pandas.read_csv(path_to_data_folder / "data.dat", delimiter=" ")
    time_list = data["time_list"].to_list()
    cmd_lost_list = data["cmd_lost_list"].to_list()
    sensor_lost_list = data["sensor_lost_list"].to_list()
    cmd_ratio_list = data["cmd_ratio_list"].to_list()
    sensor_ratio_list = data["sensor_ratio_list"].to_list()
    loop_duration_s = data["loop_duration_s"].to_list()
    ctrl_loop_duration_s = data["ctrl_loop_duration_s"].to_list()
    last_rcv_index = data["last_rcv_index"].to_list()
    cmd_index = data["cmd_index"].to_list()
    #
    latency = pandas.read_csv(path_to_data_folder / "latency.dat", delimiter=" ")[
        "latency"
    ].to_list()
    #
    hitograms = pandas.read_csv(path_to_data_folder / "histograms.dat", delimiter=" ")
    histogram_sensor = hitograms["histogram_sensor"].to_list()
    histogram_cmd = hitograms["histogram_cmd"].to_list()

    ############

    print("-- Dump latency graph --")
    plt.figure("wifi-ethernet latency", figsize=(20, 15), dpi=200)
    anchored_text = AnchoredText(
        "average latency: %f ms\nstandard deviation: %f ms"
        % (
            metadata["latency_statistics"]["average"],
            metadata["latency_statistics"]["standard_deviation"],
        ),
        loc=2,
        prop=dict(fontsize="xx-large"),
    )
    if len(latency) > 5000:
        ax1 = plt.subplot(2, 1, 1)
    else:
        ax1 = plt.subplot(1, 1, 1)
    ax1.plot(latency, ".")
    ax1.set_xlabel("index", fontsize="xx-large")
    ax1.set_ylabel("latency (ms)", fontsize="xx-large")
    ax1.add_artist(anchored_text)
    if len(latency) > 5000:
        ax2 = plt.subplot(2, 1, 2)
        ax2.plot(latency, ".")
        ax2.set_xlabel("index", fontsize="xx-large")
        ax2.set_ylabel("latency (ms)", fontsize="xx-large")
        ax2.set_ylim(-0.1, 2.1)
    if metadata["control"]["wifi_channel"] != "None":
        plt.suptitle(
            "Wifi communication latency: " + metadata["control"]["wifi_channel"],
            fontsize="xx-large",
        )
    else:
        plt.suptitle("Ethernet communication latency", fontsize="xx-large")
    plt.savefig(path_to_data_folder / "wifi_eth_latency.png")

    ############

    print("Plot histograms and graphs")
    plt.figure("wifi-ethernet statistics", figsize=(20, 15), dpi=200)
    ax3 = plt.subplot(3, 2, 1)
    ax3.bar(range(1, 21), histogram_cmd)
    ax3.set_xlabel("Number of consecutive lost command packets", fontsize="xx-large")
    ax3.set_ylabel("Number of occurences", fontsize="xx-large")
    ax3.set_xticks(range(1, 21))
    ax4 = plt.subplot(3, 2, 2)
    ax4.bar(range(1, 21), histogram_sensor)
    ax4.set_xlabel("Number of consecutive lost sensor packets", fontsize="xx-large")
    ax4.set_ylabel("Number of occurences", fontsize="xx-large")
    ax4.set_xticks(range(1, 21))
    ax5 = plt.subplot(3, 2, 3)
    ax5.plot(time_list, cmd_lost_list)
    ax5.set_xlabel("time (s)", fontsize="xx-large")
    ax5.set_ylabel("lost commands", fontsize="xx-large")
    ax6 = plt.subplot(3, 2, 4)
    ax6.plot(time_list, sensor_lost_list)
    ax6.set_xlabel("time (s)", fontsize="xx-large")
    ax6.set_ylabel("lost sensors", fontsize="xx-large")
    ax7 = plt.subplot(3, 2, 5)
    ax7.plot(time_list, cmd_ratio_list)
    ax7.set_xlabel("time (s)", fontsize="xx-large")
    ax7.set_ylabel("ratio command loss (%)", fontsize="xx-large")
    ax8 = plt.subplot(3, 2, 6)
    ax8.plot(time_list, sensor_ratio_list)
    ax8.set_xlabel("time (s)", fontsize="xx-large")
    ax8.set_ylabel("ratio sensor loss (%)", fontsize="xx-large")
    if metadata["control"]["wifi_channel"] != "None":
        plt.suptitle(
            "Wifi statistics: " + metadata["control"]["wifi_channel"],
            fontsize="xx-large",
        )
    else:
        plt.suptitle("Ethernet statistics", fontsize="xx-large")
    plt.savefig(path_to_data_folder / "wifi_eth_stats.png")

    #########

    print("Plotting the evolution of ctrl loop duration.")
    plt.figure("Ctrl loop duration", figsize=(20, 15), dpi=200)
    anchored_text = AnchoredText(
        "average duration: %f ms\nstandard deviation: %f ms"
        % (np.mean(ctrl_loop_duration_s), np.std(ctrl_loop_duration_s)),
        loc=2,
        prop=dict(fontsize="xx-large"),
    )
    ax9 = plt.subplot(1, 1, 1)
    ax9.plot(ctrl_loop_duration_s, ".")
    ax9.set_xlabel("iteration", fontsize="xx-large")
    ax9.set_ylabel("loop duration (ms)", fontsize="xx-large")
    ax9.add_artist(anchored_text)
    plt.suptitle("Command loop duration", fontsize="xx-large")
    plt.savefig(path_to_data_folder / "ctrl_command_loop_duration.png")

    #########

    print("Plotting the evolution of full loop duration.")
    plt.figure("Loop duration", figsize=(20, 15), dpi=200)
    anchored_text = AnchoredText(
        "average duration: %f ms\nstandard deviation: %f ms"
        % (np.mean(loop_duration_s), np.std(loop_duration_s)),
        loc=2,
        prop=dict(fontsize="xx-large"),
    )
    ax9 = plt.subplot(1, 1, 1)
    ax9.plot(loop_duration_s, ".")
    ax9.set_xlabel("iteration", fontsize="xx-large")
    ax9.set_ylabel("loop duration (ms)", fontsize="xx-large")
    ax9.add_artist(anchored_text)
    plt.suptitle("Command loop duration", fontsize="xx-large")
    plt.savefig(path_to_data_folder / "command_loop_duration.png")

    #########

    print("Command indexes.")
    plt.figure("Command indexes", figsize=(20, 15), dpi=200)
    plt.suptitle("Last received command index", fontsize="xx-large")
    ax = plt.subplot(2, 1, 1)
    ax.plot(cmd_index, "-")
    ax.set_xlabel("iteration", fontsize="xx-large")
    ax.set_ylabel("Command index", fontsize="xx-large")
    ax = plt.subplot(2, 1, 2)
    ax.plot(last_rcv_index, "-")
    ax.set_xlabel("iteration", fontsize="xx-large")
    ax.set_ylabel("Last received command index", fontsize="xx-large")
    plt.savefig(path_to_data_folder / "command_index.png")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Display data and dump images.")

    def dir_path(string):
        if Path(string).is_dir():
            return Path(string).resolve()
        else:
            raise NotADirectoryError(string)

    parser.add_argument(
        "path_to_data_folder",
        metavar="path_to_data_folder",
        type=dir_path,
        help="The path to the benchmark data.",
    )

    args = parser.parse_args()
    main(args.path_to_data_folder)
