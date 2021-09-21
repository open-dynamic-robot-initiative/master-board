/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, LAAS-CNRS, New York University and Max Planck
 * Gesellschaft.
 * All rights reserved.
 */

#pragma once

namespace master_board_sdk
{
namespace benchmark
{
class PD
{
public:
    PD()
    {
        position_error_ = 0.0;
        force_ = 0.0;
        set_gains(0.0, 0.0);
    }

    PD(double kp, double kd)
    {
        position_error_ = 0.0;
        force_ = 0.0;
        set_gains(kp, kd);
    }

    void set_gains(double kp, double kd)
    {
        kp_ = kp;
        kd_ = kd;
    }

    double compute(const double& position,
                   const double& velocity,
                   const double& position_target)
    {
        position_error_ = position_target - position;
        force_ = position_error_ * kp_ - velocity * kd_;
        return force_;
    }

private:
    double kp_;
    double kd_;
    double integral_;
    double integral_saturation_;

    // tmp var
    double position_error_;
    double force_;
};

}  // namespace benchmark
}  // namespace master_board_sdk
