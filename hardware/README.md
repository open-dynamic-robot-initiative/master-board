The hardware CAD file of the master-board has been created with Eagle.

# Version 1.0 (legacy)
The version 1.0 include an esp32 with GPIO connectors, 8 multiplexed SPI lines, A 5-60V to 3.3V regulator and a Ethernet PHY.
This version suffer from some limitations:
 * The 300ma current limitation of the voltage regulator is easely reach. 
 * The decoupling cappacitor could be increased.
 * The 1st pin label of DEMUX footprint is wrong
 
# Vesrion 2.0 (Latest stable version)
Chage log:
 * Add a RGB LED to ease the feedback while developping.
 * make a dedicated IMU connector
 * Switch to a more capable 3.3V voltage regulator (1A) to power 3DM-CX5-25 IMU
 
