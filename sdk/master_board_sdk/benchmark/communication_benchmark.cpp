/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, LAAS-CNRS, New York University and Max Planck
 * Gesellschaft.
 * All rights reserved.
 */

#include <iostream>
#include <vector>
#include "master_board_sdk/benchmark/communication_data.hpp"
#include "master_board_sdk/benchmark/execute_bash_command.hpp"
#include "master_board_sdk/benchmark/latency_estimator.hpp"
#include "master_board_sdk/benchmark/pd.hpp"
#include "master_board_sdk/benchmark/real_time_thread.hpp"
#include "master_board_sdk/benchmark/real_time_timer.hpp"
#include "master_board_sdk/master_board_interface.h"

using namespace master_board_sdk::benchmark;

struct ThreadSettings
{
    std::string network_interface = "";
    int ctrl_state = 0;
    int n_udriver_controlled = 1;  // Current number of controled drivers.
    double dt = 0.001;             //  Time step
    double duration_s = 20.0;      // Duration in seconds, init included
    int nb_iteration = static_cast<int>(duration_s / dt);

    // Interface objects:
    std::unique_ptr<MasterBoardInterface> robot_if_ptr;
    CommunicationData benchmark_data;
    LatencyEstimator latency_estimator;
    RtTimer loop_duration_estimator;
    RtTimer ctrl_loop_duration_estimator;
    PD pd_ctrl;
};

void* communication_benchmark(void* arg)
{
    // input parameters
    ThreadSettings* thread_settings = static_cast<ThreadSettings*>(arg);

    // Shortcuts:
    MasterBoardInterface& robot_if = *(thread_settings->robot_if_ptr);
    LatencyEstimator& latency_estimator = thread_settings->latency_estimator;
    RtTimer& loop_duration_estimator = thread_settings->loop_duration_estimator;
    RtTimer& ctrl_loop_duration_estimator = thread_settings->ctrl_loop_duration_estimator;    
    CommunicationData& benchmark_data = thread_settings->benchmark_data;
    PD& pd_ctrl = thread_settings->pd_ctrl;

    //  We enable each controler driver and its two associated motors
    for (int i = 0; i < thread_settings->n_udriver_controlled; ++i)
    {
        robot_if.GetDriver(i)->motor1->SetCurrentReference(0);
        robot_if.GetDriver(i)->motor2->SetCurrentReference(0);
        robot_if.GetDriver(i)->motor1->Enable();
        robot_if.GetDriver(i)->motor2->Enable();
        robot_if.GetDriver(i)->EnablePositionRolloverError();
        robot_if.GetDriver(i)->SetTimeout(5);
        robot_if.GetDriver(i)->Enable();
    }
    // Start sending the "get ready command"
    double last = rt_clock_sec();
    while (!robot_if.IsTimeout() && !robot_if.IsAckMsgReceived())
    {
        if ((rt_clock_sec() - last) > thread_settings->dt)
        {
            last = rt_clock_sec();
            robot_if.SendInit();
        }
    }
    // Check if the interface is properly connected.
    if (robot_if.IsTimeout())
    {
        printf("-- Timeout while waiting for ack. --\n");
        exit(0);
    }

    std::vector<double> init_pos(2 * thread_settings->n_udriver_controlled);

    printf("-- ack acquired. --\n");
    last = rt_clock_sec() - thread_settings->dt;
    int cpt = 0;  // Counting iterations.
    int cpt_print = 0;
    double start_time = 0.0;
    double current_time = 0.0;
    double motor_ref = 0.0;
    loop_duration_estimator.tic();
    while (!robot_if.IsTimeout() && thread_settings->ctrl_state != -1)
    {
        current_time = rt_clock_sec();
        // Fix the control period.
        if ((current_time - last) > thread_settings->dt)
        {
            last += thread_settings->dt;
            cpt += 1;
            cpt_print += 1;
            ctrl_loop_duration_estimator.tic();
            latency_estimator.update_received_list(
                robot_if.GetLastRecvCmdIndex(), robot_if.GetCmdPacketIndex());
            // Read sensor data sent by the masterboard
            robot_if.ParseSensorData();

            //  If the system is not ready
            if (thread_settings->ctrl_state == 0)
            {
                int ctrl_state = 1;
                // for all motors on a connected slave
                // Check if all motors are enabled and ready
                for (int i = 0; i < thread_settings->n_udriver_controlled * 2;
                     ++i)
                {
                    if (!(robot_if.GetMotor(i)->IsEnabled() &&
                          robot_if.GetMotor(i)->IsReady()))
                    {
                        ctrl_state = 0;
                    }
                }
                for (int i = 0; i < 2 * thread_settings->n_udriver_controlled;
                     ++i)
                {
                    init_pos[i] = robot_if.motors[i].get_position();
                }
                thread_settings->ctrl_state = ctrl_state;
                cpt = 0;  // Counting iterations.
                start_time = current_time;
            }
            // running control
            else
            {
                if ((current_time - start_time) > thread_settings->duration_s)
                {
                    thread_settings->ctrl_state = -1;
                    for (int i = 0;
                         i < thread_settings->n_udriver_controlled * 2;
                         ++i)
                    {
                        robot_if.GetMotor(i)->SetCurrentReference(0);
                    }
                }
                else
                {
                    for (int i = 0;
                         i < thread_settings->n_udriver_controlled * 2;
                         ++i)
                    {
                        if ((i % 2 == 0) &&
                            robot_if.GetDriver(i / 2)->error_code == 0xF)
                        {
                            continue;
                        }
                        if (robot_if.GetMotor(i)->IsEnabled())
                        {
                            motor_ref =
                                init_pos[i] +
                                4 * M_PI * sin(2 * M_PI * 0.5 * current_time);
                            robot_if.GetMotor(i)->SetCurrentReference(
                                pd_ctrl.compute(
                                    robot_if.motors[i].GetPosition(),
                                    robot_if.motors[i].GetVelocity(),
                                    motor_ref));
                        }
                    }
                }
            }
            // Display system state and communication status,
            // once every 100 iterations of the main loop
            if (cpt_print % 100 == 0)
            {
                robot_if.PrintStats();
                printf("%f > %f\n",
                       (current_time - start_time),
                       thread_settings->duration_s);
            }
            if (cpt < static_cast<int>(benchmark_data.cmd_lost_list_.size()))
            {
                benchmark_data.cmd_lost_list_[cpt] = robot_if.GetCmdLost();
                benchmark_data.sensor_lost_list_[cpt] =
                    robot_if.GetSensorsLost();
                benchmark_data.cmd_ratio_list_[cpt] =
                    100.0 * static_cast<double>(robot_if.GetCmdLost()) /
                    static_cast<double>(robot_if.GetCmdSent());
                benchmark_data.sensor_ratio_list_[cpt] =
                    100.0 * static_cast<double>(robot_if.GetSensorsLost()) /
                    static_cast<double>(robot_if.GetSensorsSent());
                benchmark_data.time_list_[cpt] = current_time - start_time;
                benchmark_data.last_rcv_index_[cpt] = robot_if.GetLastRecvCmdIndex();
                benchmark_data.cmd_index_[cpt] = robot_if.GetCmdPacketIndex();
            }
            latency_estimator.update_sent_list(robot_if.GetCmdPacketIndex());
            // Send the reference currents to the master board
            robot_if.SendCommand();
            double ctrl_loop_duration = ctrl_loop_duration_estimator.tac();
            double loop_duration = loop_duration_estimator.tac_tic();
            if (cpt < static_cast<int>(benchmark_data.loop_duration_s_.size()))
            {
                benchmark_data.loop_duration_s_[cpt] = loop_duration;
                benchmark_data.ctrl_loop_duration_s_[cpt] = ctrl_loop_duration;
            }
        }
    }
    // Shut down the interface between the computer and the master
    // board
    robot_if.Stop();
    if (robot_if.IsTimeout())
    {
        printf(
            "-- Masterboard timeout detected. --\n"
            "-- "
            "Either the masterboard has been shutdown, "
            "or there has been a connection issue with the cable/wifi."
            " --\n");
    }
    benchmark_data.latency_ = latency_estimator.get_latency_measurement();
    // Plot histograms and graphs
    for (int i = 0;
            i < static_cast<int>(benchmark_data.histogram_sensor_.size());
            ++i)
    {
        benchmark_data.histogram_sensor_[i] =
            robot_if.GetSensorHistogram(i);
        benchmark_data.histogram_cmd_[i] = robot_if.GetCmdHistogram(i);
    }
    printf("-- Dump the collected data --\n");
    benchmark_data.dump_data();
    return nullptr;
}

int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        throw std::runtime_error(
            "Wrong number of argument, please add: 'the network interface "
            "name' 'the distance between the mb and the pc' 'the wifi card name'.");
    }
    ThreadSettings thread_settings;

    thread_settings.network_interface = argv[1];
    thread_settings.robot_if_ptr = std::make_unique<MasterBoardInterface>(
        thread_settings.network_interface);
    // Initialization of the interface between the computer and the master board
    thread_settings.robot_if_ptr->Init();
    thread_settings.latency_estimator.initialize(
        thread_settings.robot_if_ptr->GetCmdPacketIndex());
    thread_settings.benchmark_data.initialize(
        thread_settings.nb_iteration,
        thread_settings.dt,
        thread_settings.network_interface,
        thread_settings.robot_if_ptr->GetProtocolVersion(),
        thread_settings.duration_s,
        std::atof(argv[3]),
        argv[2]);
    thread_settings.pd_ctrl.set_gains(0.2, 0.05);

    printf("Executing the Benchmark.\n");
    thread_settings.ctrl_state = 0;
    RealTimeThread rt_thread;
    rt_thread.create_realtime_thread(&communication_benchmark,
                                     &thread_settings);
    rt_thread.join();

    printf("Extract the plots from the collected data.");
    std::string output;
    {
        std::ostringstream oss;
        oss << argv[0] << "_plotter "
            << thread_settings.benchmark_data.data_folder_;
        execute_bash_command(oss.str());
    }
    printf("End of program!\n");
    return 0;
}
