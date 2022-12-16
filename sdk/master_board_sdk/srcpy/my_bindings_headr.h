#ifndef MYBINDINGSHEADR_H
#define MYBINDINGSHEADR_H

// Silence a warning about a deprecated use of boost bind by boost python
// at least fo boost 1.73 to 1.76
// ref. https://github.com/stack-of-tasks/tsid/issues/128
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#include <boost/python.hpp>
#undef BOOST_BIND_GLOBAL_PLACEHOLDERS

#include "master_board_sdk/Link_manager.h" // Link_manager class
#include "master_board_sdk/master_board_interface.h" // MasterBoardInterface class
#include "master_board_sdk/motor.h" // Motor class
#include "master_board_sdk/motor_driver.h" // MotorDriver class

#endif
