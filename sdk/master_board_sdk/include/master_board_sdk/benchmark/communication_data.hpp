/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, LAAS-CNRS, New York University and Max Planck
 * Gesellschaft.
 * All rights reserved.
 */

#pragma once

#include <boost/filesystem.hpp>
#include <deque>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>
#include "master_board_sdk/benchmark/execute_bash_command.hpp"
#include "master_board_sdk/benchmark/real_time_timer.hpp"

namespace master_board_sdk
{
namespace benchmark
{
class CommunicationData
{
public:
    CommunicationData()
    {
    }

    ~CommunicationData()
    {
        printf("Delete CommunicationData!\n");
        fflush(stdout);
    }

    void initialize(const int& size,
                    const double& dt,
                    const std::string& network_interface,
                    const int& protocol_version,
                    const double& experiment_duration,
                    const double& distance,
                    const std::string& wifi_card_name)
    {
        // Data storage.
        time_list_.resize(size, 0.0);
        last_rcv_index_.resize(size, 0);
        cmd_index_.resize(size, 0);
        cmd_lost_list_.resize(size, 0);
        sensor_lost_list_.resize(size, 0);
        cmd_ratio_list_.resize(size, 0.0);
        sensor_ratio_list_.resize(size, 0.0);
        latency_.clear();
        loop_duration_s_.resize(size, 0.0);
        ctrl_loop_duration_s_.resize(size, 0.0);

        // Histograms
        histogram_sensor_.resize(20, 0);
        histogram_cmd_.resize(20, 0);

        // Data folder to dump the data/figures.
        data_folder_ = get_log_dir("master_board_sdk");

        // Metadata and statistics.
        control_period_ = dt;
        network_interface_ = network_interface;
        experiment_duration_ = experiment_duration;
        distance_ = distance;
        wifi_card_name_ = wifi_card_name;
        if (network_interface_.at(0) == 'w')
        {
            wifi_channel_ = execute_bash_command(
                "iwlist " + network_interface_ + " channel | grep Frequency");
        }
        else
        {
            wifi_channel_ = "None";
        }
        distribution_ = execute_bash_command("uname -a");
        protocol_version_ = protocol_version;
        latence_average_ = 0.0;
        latence_stdev_ = 0.0;
    }

    void get_latency_statistics(const std::deque<double>& latency,
                                double& average,
                                double& stdev)
    {
        std::vector<double> latency_pruned;
        latency_pruned.reserve(latency.size());
        for (std::size_t i = 0; i < latency.size(); ++i)
        {
            if (latency[i] > 0.0)
            {
                latency_pruned.push_back(latency[i]);
            }
        }

        std::vector<double>& v = latency_pruned;
        double sum = std::accumulate(v.begin(), v.end(), 0.0);
        average = sum / static_cast<double>(v.size());

        std::vector<double> diff(v.size());
        std::transform(v.begin(),
                       v.end(),
                       diff.begin(),
                       std::bind2nd(std::minus<double>(), average));
        double sq_sum =
            std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
        stdev = std::sqrt(sq_sum / static_cast<double>(v.size()));
    }

    void dump_data()
    {
        // Writing in files
        std::ofstream myfile;

        // Storing data
        myfile.open(data_folder_ + "/" + "data.dat");
        myfile << "time_list "
               << "cmd_lost_list "
               << "sensor_lost_list "
               << "cmd_ratio_list "
               << "sensor_ratio_list "
               << "loop_duration_s "
               << "ctrl_loop_duration_s "
               << "last_rcv_index "
               << "cmd_index " << std::endl;
        for (std::size_t i = 0; i < time_list_.size(); ++i)
        {
            myfile << time_list_[i] << " "             //
                   << cmd_lost_list_[i] << " "         //
                   << sensor_lost_list_[i] << " "      //
                   << cmd_ratio_list_[i] << " "        //
                   << sensor_ratio_list_[i] << " "     //
                   << loop_duration_s_[i] << " "       //
                   << ctrl_loop_duration_s_[i] << " "  //
                   << last_rcv_index_[i] << " "        //
                   << cmd_index_[i] << " "             //
                   << std::endl;
        }
        myfile.close();

        myfile.open(data_folder_ + "/" + "latency.dat");
        myfile << "latency" << std::endl;
        for (std::size_t i = 0; i < latency_.size(); ++i)
        {
            myfile << latency_[i] << std::endl;
        }
        myfile.close();

        // Storing the Histograms
        myfile.open(data_folder_ + "/" + "histograms.dat");
        myfile << "histogram_sensor histogram_cmd" << std::endl;
        for (std::size_t i = 0; i < histogram_sensor_.size(); ++i)
        {
            myfile << histogram_sensor_[i] << " " << histogram_cmd_[i]
                   << std::endl;
        }
        myfile.close();

        // metadata
        get_latency_statistics(latency_, latence_average_, latence_stdev_);
        myfile.open(data_folder_ + "/" + "meta_data.yaml");
        myfile << "control:" << std::endl
               << "    controller_period: " << 0.001 << std::endl
               << "    network_interface: " << network_interface_ << std::endl
               << "    wifi_channel: " << wifi_channel_ << std::endl
               << "    experiment_duration: " << experiment_duration_
               << std::endl
               << "    distance: " << distance_ << std::endl
               << "latency_statistics:" << std::endl
               << "    average: " << latence_average_ << std::endl
               << "    standard_deviation: " << latence_stdev_ << std::endl
               << "system:" << std::endl
               << "    wifi_card_name: " << wifi_card_name_ << std::endl
               << "    distribution_: " << distribution_ << std::endl
               << "    date_time: " << date_ << std::endl
               << "    protocole_version: '3'" << std::endl;
        myfile.close();
        printf("Data dumped in %s\n", data_folder_.c_str());
        fflush(stdout);
    }

    std::string get_log_dir(const std::string& app_name)
    {
        std::string home_dir = std::string(getenv("HOME")) + "/";
        date_ = RtTimer::get_current_date_str();
        std::string log_dir = home_dir + app_name + "/" + date_ + "/";

        boost::filesystem::create_directory(home_dir + app_name);
        boost::filesystem::create_directory(log_dir);

        return log_dir;
    }

    // Histograms
    std::vector<int> histogram_sensor_;
    std::vector<int> histogram_cmd_;

    // Data
    std::vector<int> last_rcv_index_;
    std::vector<int> cmd_index_;
    std::vector<int> cmd_lost_list_;
    std::vector<int> sensor_lost_list_;
    std::vector<double> time_list_;
    std::vector<double> cmd_ratio_list_;
    std::vector<double> sensor_ratio_list_;
    std::deque<double> latency_;
    std::vector<double> loop_duration_s_;
    std::vector<double> ctrl_loop_duration_s_;

    // data_folder
    std::string data_folder_;

    // metadata controller
    double control_period_;
    std::string network_interface_;
    std::string wifi_channel_;
    double experiment_duration_;

    // metadata latency
    double latence_average_;
    double latence_stdev_;

    // metadata system
    std::string wifi_card_name_;
    double distance_;
    std::string distribution_;
    int protocol_version_;
    std::string date_;
};

}  // namespace benchmark
}  // namespace master_board_sdk
