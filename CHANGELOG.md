# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Unreleased]

## [1.0.6] - 2023-11-15

-    Change name of the package to make it release in ROS-2.
-    Partial support of the new Power-Board hardware
-    Fix two bugs (FLOAT_TO_D8QN and compiling issue) + remove warnings on the core library.
-    [protocol] FLOAT_TO_D8QN has a wrong bracket. This commit fix this.
-    [cmake] Synchronize to remove error during install process.
-    Add error, thrown by sdk, for protocol version mismatch (during init)
-    Add ament_cmake build_type export in package.xml and install package.xml file
-    Update sdk/master_board_sdk/CMakeLists.txt
-    [package.xml] Add dependence to ament_cmake if ROS-2 is detected.
-    [CMakeLists.txt] Uses ROS-2 if the environment is compatible.
-    [package.xml] Add ament_cmake export for colcon support.
-    [cmake] Add install procedure for file package.xml
-    Adding ROS-2 rolling CI to the repository and some changes to have it working.
-    [sdk/master_board_sdk/tests/test_protocol.cpp] Commented the test on NaN which is failing.
-    [sdk/master_board_sdk/package.xml] Fix package dependency. Update version to 3. Add dependencies to libboost-python. Add git for dependencies.
-    [cmake] Change place for python detection
-    [package.xml] XML version 1.0
-    [package.xml] Remove Boost depend.
-    (Small) Fix for sdk examples with python >=3.8
-    Apple support for wired connection.
-    [Link_manager] Fix message when the priority is not set correctly and fails using assert.
-    Add Python bindings for SetKp/Kd/SaturationCurrent and fix example.py
-    Zero-initialise all members of Motor
-    Change macro name for platform from UNIX to linux. Include net/if_arp.h only for APPLE.
-    example.py: Initialise all reference values and gains to zero This should not really be needed anymore since values are already zero-initialised in Motor but better be safe than sorry.
-    fix example.py: time.clock() has been removed time.clock() has been deprecated in Python 3.3 and does not exist anymore in newer versions. Use perf_counter instead.
-    Add Python bindings for Motor::SetKp/Kd/SaturationCurrent At least by now they are implemented in motor.cpp, so there's no reason to not add bindings for them.
-    Zero-initialise all members of Motor This should fix an issue that was likely caused by kp being set to some random non-zero value, resulting in the motor unintentionally being held at some position.
-    plateform and distro modules optionnal for sdk example com_analyser.py
-    Swap process_time and clock for better intelligibility
-    Protocol version added in init_ack packet & version mismatch throw runtime error in sdk
-    Fix sdk examples for python version >=3.8
-    CMake: add unit tests
-    Handle out-of-range motor params without wraparound (#117) Avoid fixed-point overflow
-    [CMake] set RPATH for executables
-    Add Python bindings for powerboard data
-    add pyton bindings for power baord
-    [SDK][Firmware] Add partial support for power-board, Update protocol version and sdk accordingly
-    Contributors: EtienneAr, Felix Kloss, Guilhem Saurel, Naveau, Olivier Stasse, Thomas Flayols, Trevor Blackwell, odri, thomasfla


## [1.0.5] - 2022-06-29

### SDK
#### Changed
- Use `ip` instead of `ifconfig` in `setup_wifi.sh`

#### Fixed
- Fix package.xml for catkin build compatibility.

### Firmware
#### Fixed
- Use mailbox for SPI communication.  This fixes timing issues that occurred in
  some situations (e.g. when using wifi).


## [1.0.4] - 2021-07-21

There is no changelog for this or earlier versions.


[Unreleased]: https://github.com/open-dynamic-robot-initiative/master-board/compare/v1.0.5...HEAD
[1.0.5]: https://github.com/open-dynamic-robot-initiative/master-board/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/open-dynamic-robot-initiative/master-board/releases/tag/v1.0.4
