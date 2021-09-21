/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, LAAS-CNRS, New York University and Max Planck
 * Gesellschaft.
 * All rights reserved.
 */

#pragma once

#include <deque>
#include "master_board_sdk/benchmark/real_time_timer.hpp"

namespace master_board_sdk
{
namespace benchmark
{
class LatencyEstimator
{
public:
    LatencyEstimator()
    {
        start_ = false;
        overflow_cmd_cpt_ = 0;
        first_cmd_index_ = 0;
        cmd_packet_index_ = 0;
        last_cmd_packet_index_ = 0;

        sent_list_.clear();
        received_list_.clear();
        latency_.clear();
    }

    ~LatencyEstimator()
    {
        printf("Delete LatencyEstimator!\n");
        fflush(stdout);
    }

    void initialize(int last_recv_cmd_index)
    {
        overflow_cmd_cpt_ = 0;
        first_cmd_index_ = last_recv_cmd_index;
        cmd_packet_index_ = last_recv_cmd_index;
        last_cmd_packet_index_ = last_recv_cmd_index;
        start_ = true;
    }

    void update_received_list(int last_recv_cmd_index,
                              int current_cmd_packet_index)
    {
        int index = 0;
        if (last_recv_cmd_index > current_cmd_packet_index)
        {
            index = (overflow_cmd_cpt_ - 1) * 65536 + last_recv_cmd_index;
        }
        else
        {
            index = overflow_cmd_cpt_ * 65536 + last_recv_cmd_index;
        }
        index -= first_cmd_index_;

        if (index >= 0)
        {
            // We make sure to resize the deque properly before affecting the
            // values.
            while (index >= static_cast<int>(received_list_.size()))
            {
                received_list_.push_back(0.0);
            }
            if (received_list_[index] == 0)
            {
                received_list_[index] = rt_clock_sec();
            }
        }
    }

    void update_sent_list(int current_cmd_packet_index)
    {
        int diff = current_cmd_packet_index - last_cmd_packet_index_;
        if (diff < 0)
        {
            overflow_cmd_cpt_ += 1;
            diff = 65536 + current_cmd_packet_index - last_cmd_packet_index_;
        }
        cmd_packet_index_ += diff;
        last_cmd_packet_index_ = current_cmd_packet_index;
        int index = cmd_packet_index_ - first_cmd_index_;
        assert(index >= 0);

        // We make sure to resize the deque properly before affecting the
        // values.
        while (index >= static_cast<int>(sent_list_.size()))
        {
            sent_list_.push_back(0.0);
        }
        sent_list_[index] = rt_clock_sec();
    }

    const std::deque<double>& get_latency_measurement()
    {
        latency_ = std::deque<double>(
            std::min(sent_list_.size(), received_list_.size()), 0.0);

        for (std::size_t i = 0;
             ((i < sent_list_.size()) && (i < received_list_.size()) &&
              (i < latency_.size()));
             ++i)
        {
            if (received_list_[i] != 0 && sent_list_[i] != 0)
            {
                latency_[i] = 1000 * (received_list_[i] - sent_list_[i]);
            }
            else
            {
                latency_[i] = 0.0;  // 0 means not sent or not received
            }
        }
        return latency_;
    }

private:
    bool start_;
    int overflow_cmd_cpt_;
    int first_cmd_index_;
    int cmd_packet_index_;
    int last_cmd_packet_index_;
    std::deque<double> sent_list_;
    std::deque<double> received_list_;
    std::deque<double> latency_;
};

}  // namespace benchmark
}  // namespace master_board_sdk
