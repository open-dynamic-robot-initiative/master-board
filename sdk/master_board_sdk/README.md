# SDK
The SDK provide a simple C++ class interface to send command to the masterboard via Ethernet or Wifi.

## Prepare you interface
You first need to identify you interface name. To get a list of the interface on your computer, run ```ifconfig```

#### Ethernet 
A direct connection should me made between the master board and you computer. It is not possible to use the same interface for local network and master board connection.

###### Optional configuration:
Some packet will be sent by the OS to the master board. Their are not usefull since the master board only use raw MAC frame.
To disable ARP packet run:
```
sudo ifconfig MY_INTERFACE -arp
``` 
where MY_INTERFACE is your lan interface name.

To disable IPV6 trafic, add this line into ``` /etc/sysctl.conf``` 
``` 
net.ipv6.conf.MY_INTERFACE.disable_ipv6 = 1
``` 
where MY_INTERFACE is your lan interface name.
#### Wifi
Your interface should support monitor mode and injection since the procol used by the master board is not a standard wifi.
You need to configure your interface. A script is available in the sdk folder. to use it run 
```
sudo ./setup_wifi.sh MY_INTERFACE
``` 
where MY_INTERFACE your wlan interface name.

How to run the C++ example
--------
main.cpp is a simple example to test the SDK, tested on ubuntu.
It will execute a sinusoid trajectory on the first N_CONTROLLED_SLAVE 
to compile it go into the sdk folder and run:
```
make
```
a bin folder is created.
to run the example run:
```
sudo ./bin/exec MY_INTERFACE
```
where MY_INTERFACE is the name of the network interface used to connect to the master board.

How to run the Python examples
--------

* Clone the repository: `git clone --recursive https://github.com/open-dynamic-robot-initiative/master-board.git`

* Get into the repository: `cd master-board`

* Set the number of controlled drivers by opening `sdk/master_board_sdk/example/example.py` and tuning the `N_SLAVES_CONTROLED` constant: `N_SLAVES_CONTROLED 4` if you are using 4 driver boards (1 per leg)

* Get in `sdk/master_board_sdk/`: `cd sdk/master_board_sdk/`

* Create a build folder: `mkdir build`

* Get into the folder: `cd build`

* Two possibilities:
    * Using `ccmake ..` turn on Python bindings by setting `BUILD_PYTHON_INTERFACE` to `ON` and `CMAKE_BUILD_TYPE` to `RELEASE`. Then compile and create the bindings: `cmake ..` then `make`
    * Directly use `cmake -DBUILD_PYTHON_INTERFACE=ON -DCMAKE_BUILD_TYPE=RELEASE ..` then `make`. If you want to run the scripts with Python 3 then use `cmake -DBUILD_PYTHON_INTERFACE=ON -DCMAKE_BUILD_TYPE=Release -DPYTHON_EXECUTABLE=$(which python3) ..` instead.

* Run the control script with the name of your Ethernet interface instead of `name_interface` (for instance `enp1s0`): 

`sudo PYTHONPATH=. python example/example.pyc -i name_interface`

* Run the listener script with the name of your Ethernet interface instead of `name_interface` (for instance `enp1s0`): 

`sudo PYTHONPATH=. python example/listener.pyc -i name_interface`

* Run the latency calculator script with the name of your Ethernet interface instead of `name_interface` (for instance `enp1s0`): 

`sudo PYTHONPATH=. python example/latency.pyc -i name_interface`


How to run an executable based on the SDK without root permissions
-------

To run an executable named EXECUTABLE_NAME without root permissions, run :
```
sudo setcap cap_net_admin,cap_net_raw+ep EXECUTABLE_NAME
```
