# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Unreleased]


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
