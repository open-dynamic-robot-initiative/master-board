# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.7] - 2024-06-24

### cmake
- Do not fetch Catch2 look for the related distribution's package.
- Fix python bindings problems.
- Fix wrong inclusion.

## [1.0.6] - 2023-12-15

### SDK

#### cmake
- bump catch2
- sync submodule
- Bump ros-industrial/industrial_ci
	
#### Fixed
- Fix two bugs (FLOAT_TO_D8QN and compiling issue) + remove warnings on the core library.
- Fix warnings. There are still some warnings wih Catch2 and clang. But Catch2 is a dependency and should be fixed upstream.
- [protocol] FLOAT_TO_D8QN has a wrong bracket. 
- Add error, thrown by sdk, for protocol version mismatch (during init)
- Add ROS-2 support for colcon
- Adding ROS-2 rolling CI to the repository and some changes to have it working (many fixes and update by @nim65s and @mnaveau)
- [Link_manager] Fix message when the priority is not set correctly and fails using assert.
- Add Python bindings for SetKp/Kd/SaturationCurrent and fix example.py
- Zero-initialise all members of Motor
- [sdk/master_board_sdk/tests/test_protocol.cpp] Commented the test on NaN which is failing.
- Add APPLE compatibility for wired connection (no wifi support yet)
- example.py: Initialise all reference values and gains to zero
This should not really be needed anymore since values are already
zero-initialised in `Motor` but better be safe than sorry.
fix example.py: time.clock() has been removed
`time.clock()` has been deprecated in Python 3.3 and does not exist
anymore in newer versions.  Use `perf_counter` instead.
- Add Python bindings for Motor::SetKp/Kd/SaturationCurrent
At least by now they are implemented in motor.cpp, so there's no reason
to not add bindings for them.
- Zero-initialise all members of Motor
This should fix an issue that was likely caused by `kp` being set to
some random non-zero value, resulting in the motor unintentionally being
held at some position.
- Update masterboard_communication.md

### Firmware
- Update BLMC_µDriver_SPI_interface.md
- Partial support of the new Power-Board hardware
- fixed typos in firmware README.md
- updated documentation for flashing the master board
- added more details to firmware README.md
- Handle out-of-range motor params without wraparound (#117)
- Avoid fixed-point overflow

### Contributors (by alphabetical order)
- Etienne Arlaud
- Trevor Blackwell
- Thomas Flayols
- Felix Kloss
- Guilhem Saurel
- Olivier Stasse
- Huaijiang Zhu

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


[Unreleased]: https://github.com/open-dynamic-robot-initiative/master-board/compare/v1.0.6...HEAD
[1.0.5]: https://github.com/open-dynamic-robot-initiative/master-board/compare/v1.0.5...v1.0.6
[1.0.5]: https://github.com/open-dynamic-robot-initiative/master-board/compare/v1.0.4...v1.0.5
[1.0.4]: https://github.com/open-dynamic-robot-initiative/master-board/releases/tag/v1.0.4
