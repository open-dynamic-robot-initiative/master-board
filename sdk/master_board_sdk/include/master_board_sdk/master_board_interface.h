#ifndef MASTERBOARDINTERFACE_H
#define MASTERBOARDINTERFACE_H

#include <mutex>

#include "master_board_sdk/ESPNOW_manager.h"
#include "master_board_sdk/ETHERNET_manager.h"
#include "master_board_sdk/defines.h"
#include "master_board_sdk/motor_driver.h"
#include "master_board_sdk/motor.h"

#define MAX_HIST 20

class MasterBoardInterface : public LINK_manager_callback
{
public:
	MasterBoardInterface(const std::string &if_name);
	MasterBoardInterface(const MasterBoardInterface&);
	int Init();
	int Stop();
	void SetMasterboardTimeoutMS(uint8_t); //Set the Master board timeout in ms
	void SendCommand();										 //Send the command packet to the master board
	void ParseSensorData();								 //Parse and convert the latest received sensor data. User need to call this before reading any field.
	void PrintIMU();											 //Print IMU data on stdout. Usefull for debug.
	void PrintADC();											 //Print ACD data on stdout. Usefull for debug.
	void PrintMotors();										 //Print motors data on stdout. Usefull for debug.
	void PrintMotorDrivers();							 //Print motor drivers data on stdout. Usefull for debug.  void PrintMotors(); //Print Motors data on stdout. Usefull for debug.

  	uint16_t nb_recv = 0; //todo make private
private:
	void callback(uint8_t src_mac[6], uint8_t *data, int len);
	uint8_t my_mac_[6];		// = {0xa0, 0x1d, 0x48, 0x12, 0xa0, 0xc5};	 //{0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
	uint8_t dest_mac_[6]; //Broatcast to prevent acknoledgment behaviour
	LINK_manager *link_handler_;
	uint8_t payload_[127];
	std::string if_name_;
	struct command_packet_t command_packet;
	struct sensor_packet_t sensor_packet;
	struct dual_motor_driver_sensor_data_t dual_motor_driver_sensor_data[N_SLAVES];
	struct imu_data_t imu_data;

	int histogram_lost_sensor_packets[MAX_HIST]; //histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...
	int histogram_lost_cmd_packets[MAX_HIST];		 //histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...

	uint16_t last_sensor_index = 0;
	uint32_t nb_sensors_sent = 0; //this variable deduce the total number of received sensor packet from sensor index and previous sensor index
	uint32_t nb_sensors_lost = 0;

	uint32_t nb_cmd_lost_offset = -1;
	uint32_t last_cmd_lost = 0;

	std::mutex sensor_packet_mutex;
public:
	Motor motors[N_SLAVES * 2];
	MotorDriver motor_drivers[N_SLAVES];

	// Set functions for Python binding with Boost
	void set_nb_recv(uint16_t val) { this->nb_recv = val; };
	void set_motors(Motor motors []); // See definition in .cpp
	void set_motor_drivers(MotorDriver motor_drivers []); // See definition in .cpp

	// Get functions for Python binding with Boost
	uint16_t get_nb_recv() { return this->nb_recv; };
	Motor* get_motors() { return this->motors; };
	MotorDriver* get_motor_drivers() { return this->motor_drivers; };
	MotorDriver* GetDriver(int i) { return &(this->motor_drivers[i]); };
	Motor* GetMotor(int i) { return &(this->motors[i]); };

};

#endif
