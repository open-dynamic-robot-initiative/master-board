# master-board
Hardware and Firmware of the Solo Quadruped Master Board.

This board centralises all the sensor and actuator data and provides wired and wireless connection to a realtime computer.

Connectivity:

* SPI: Address up to 8 SPI Slave: (max 80Mhz, DMA capable) compatible with BLMC µDriver SPI interface
* Wifi: Wireless communication with a computer via raw ESP-NOW: round trip time of 1.2ms (including driver and OS latency) for a 127bytes message.
* Ethernet: Wired communication with a computer via raw frames: round trip time of 0.2ms (including driver and OS latency) for a 127bytes message.
* GPIO: 4GPIO free. Can be mapped to I2C, UART etc.. Two of them are curently used for IMU communication via UART
* UART: Used to upgrade the ESP32 firmware, free on normal operation.

The board is programed via the ESP-IDF tool chain https://github.com/espressif/esp-idf

Wireless closed loop control at 1kHz demo (click to see video):
[![Alt text](https://img.youtube.com/vi/kEtmWzfE4aw/0.jpg)](https://www.youtube.com/watch?v=kEtmWzfE4aw)

IMU, ethernet closed loop cntrol at 1kHz demo (click to see video):
[![Alt text](https://img.youtube.com/vi/TaonDmPJcGE/0.jpg)](https://www.youtube.com/watch?v=TaonDmPJcGE)


Documentation
-------------
Here are some helpful links to the documentation :

[How to flash the Master Board (install esp-idf and flash the firmware)](firmware/README.md)

[SDK : How to prepare your interface and run the example](sdk/master_board_sdk/README.md )

[Master Board State Machine Description](documentation/masterboard_state_machine.md)

[Description of the communication between the Master Board and the computer](documentation/masterboard_communication.md)

[Description of the BLMC µDriver SPI interface](documentation/BLMC_%C2%B5Driver_SPI_interface.md)

[Wiring the MasterBoard](documentation/masterboard_wiring.md)

[Master Board Ordering and Preparation](documentation/masterboard_ordering_soldering.md)

Authors
--------
Thomas Flayols  
Etienne Arlaud

License
-------
BSD 3-Clause License

Copyright
-----------
Copyright (c) 2019, LAAS-CNRS, Max Planck Gesellschaft, New York University

More Information
----------------
[Open Dynamic Robot Initiative](https://open-dynamic-robot-initiative.github.io)  
