# SDK
The SDK provide a simple C++ class interface to send command to the masterboard via Ethernet or Wifi.

## Prepare you interface
You first need to identify you interface name. To get a list of the interface on your computer, run ```ifconfig```

#### Ethernet 
A direct connection should me made between the master board and you computer. It is not possible to use the same interface for local network and master board connection.

#### Wifi
Your interface should support monitor mode and injection since the procol used by the master board is not a standard wifi.
You need to configure your interface. A script is available in the sdk folder. to use it run 
```
sudo ./setup_wifi.sh MY_INTERFACE
``` 
where MY_INTERFACE your wlan interface name.

How to run the example
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
