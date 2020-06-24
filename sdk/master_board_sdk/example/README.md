See README.md in sdk folder for instructions on how to run the examples.

Control examples
---
**Control** examples are called **example**, they run a simple control script for the test benches.

Listener example
---
**Listener** example is called **listener**, it runs a script that detects a running master board and listens to its incoming packets, printing useful stats. It is not able to send commands.

Communication analyser example
---
**Communication analyser** example is called **com_analyser**, it runs a script that computes the latency of the wifi/ethernet communication between the computer and the master board, and plots the evolution of packet losses and the command loop duration. The graphs are stored in 'master_board_sdk/graphs' with a text file containing the system info, in a folder named from the current date and time. It sends null torques as command.
