/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, LAAS-CNRS, New York University and Max Planck
 * Gesellschaft.
 * All rights reserved.
 */

#pragma once

#include <array>
#include <string>

namespace master_board_sdk
{
namespace benchmark
{
std::string execute_bash_command(const std::string& cmd)
{
    auto pPipe = ::popen(cmd.c_str(), "r");
    if (pPipe == nullptr)
    {
        throw std::runtime_error("Cannot open pipe");
    }

    std::array<char, 256> buffer;

    std::string result;

    while (!std::feof(pPipe))
    {
        auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
        result.append(buffer.data(), bytes);
    }

    ::pclose(pPipe);
    return result;
}

}  // namespace benchmark
}  // namespace master_board_sdk
