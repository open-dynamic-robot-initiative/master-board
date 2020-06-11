#ifndef MASTERBOARDINTERFACE_H
#define MASTERBOARDINTERFACE_H

#include <mutex>
#include <chrono>

#include "master_board_sdk/ESPNOW_manager.h"
#include "master_board_sdk/ETHERNET_manager.h"
#include "master_board_sdk/defines.h"
#include "master_board_sdk/motor_driver.h"
#include "master_board_sdk/motor.h"

#define MAX_HIST 20

class MasterBoardInterface : public LINK_manager_callback
{
public:
	MasterBoardInterface(const std::string &if_name, bool listener_mode = false);
	MasterBoardInterface(const MasterBoardInterface&);
	~MasterBoardInterface();
	int Init();
	int Stop();
	static void KeyboardStop(int signum);
	void SetMasterboardTimeoutMS(uint8_t); //Set the Master board timeout in ms
	int SendInit();
	int SendCommand();										 //Send the command packet to the master board

	void ParseSensorData();								 //Parse and convert the latest received sensor data. User need to call this before reading any field.
	
	void PrintIMU();											 //Print IMU data on stdout. Usefull for debug.
	void PrintADC();											 //Print ACD data on stdout. Usefull for debug.
	void PrintMotors();										 //Print motors data on stdout. Usefull for debug.
	void PrintMotorDrivers();							 //Print motor drivers data on stdout. Usefull for debug.  void PrintMotors(); //Print Motors data on stdout. Usefull for debug.
	void PrintSensorStats();
	void PrintCmdStats();

	void ResetTimeout();  // Reset the interface after at timeout to send packets again
	bool IsTimeout();     // Check if a timeout has been triggered because the master board did not respond

	bool IsAckMsgReceived();

	int GetSessionId();

	uint32_t GetSensorsSent();
	uint32_t GetSensorsLost();
	uint32_t GetCmdSent();
	uint32_t GetCmdLost();
	uint16_t GetLastRecvCmdIndex();
	uint16_t GetCmdPacketIndex();
	int GetSensorHistogram(int index);
	int GetCmdHistogram(int index);

	void ResetPacketLossStats();

private:
	static MasterBoardInterface* instance;
	void callback(uint8_t src_mac[6], uint8_t *data, int len);
	uint8_t my_mac_[6];		// = {0xa0, 0x1d, 0x48, 0x12, 0xa0, 0xc5};	 //{0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
	uint8_t dest_mac_[6]; //Broatcast to prevent acknoledgment behaviour
	LINK_manager *link_handler_ = NULL;
	uint8_t payload_[127];
	std::string if_name_;
	bool listener_mode = false; // listener mode, allows to gather sensor packets data and to get rid of session id checking
								// doesn't allow to send commands
	struct command_packet_t command_packet;
	struct sensor_packet_t sensor_packet;
	struct dual_motor_driver_sensor_data_t dual_motor_driver_sensor_data[N_SLAVES];
	struct imu_data_t imu_data;

	bool first_sensor_received = false;

	// PACKET LOSS STATS
	//SENSOR PACKETS
  	uint16_t nb_sensors_recv = 0;
	uint16_t last_sensor_index = 0; 
	uint32_t nb_sensors_sent = 0;
	uint32_t nb_sensors_lost = 0;
	uint16_t last_recv_cmd_index = 0;
	int histogram_lost_sensor_packets[MAX_HIST]; //histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...


	//COMMAND PACKETS
	uint16_t cmd_packet_index = 0;
	uint16_t last_cmd_packet_loss = 0;
	uint32_t nb_cmd_sent = 0;
	uint32_t nb_cmd_lost = 0;
	int histogram_lost_cmd_packets[MAX_HIST];	//histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...


	std::mutex received_packet_mutex;

	// Time duration [ms] after which the MasterBoardInterface shuts down if the
	// master board is not responding while waiting for acknowledge msg (timeout)
	std::chrono::milliseconds t_before_shutdown_ack{2500};

	// Time duration [ms] after which the MasterBoardInterface shuts down if the
	// master board is not responding (timeout)
	std::chrono::milliseconds t_before_shutdown_control{50};

	// Time point that is updated each time a packet is received
	std::chrono::high_resolution_clock::time_point t_last_packet =
            std::chrono::high_resolution_clock::now();
	// We initialize this value only upon the first call of SendCommand
    bool first_command_sent_ = false;


	// Is true if the MasterBoardInterface has been shut down due to timeout
	bool timeout = false;

	struct init_packet_t init_packet;
	struct ack_packet_t ack_packet;

	int session_id = -1; // -1 means not set

	void GenerateSessionId();

	bool init_sent = false;
	bool ack_received = false;

public:
	Motor motors[N_SLAVES * 2];
	MotorDriver motor_drivers[N_SLAVES];

	// Set functions for Python binding with Boost
	void set_motors(Motor motors []); // See definition in .cpp
	void set_motor_drivers(MotorDriver motor_drivers []); // See definition in .cpp

	// Get functions for Python binding with Boost
	MotorDriver* GetDriver(int i) { return &(this->motor_drivers[i]); };
	Motor* GetMotor(int i) { return &(this->motors[i]); };
	float imu_data_accelerometer(int i) { return (this->imu_data.accelerometer[i]); };
	float imu_data_gyroscope(int i) { return (this->imu_data.gyroscope[i]); };
	float imu_data_attitude(int i) { return (this->imu_data.attitude[i]); };
	float imu_data_linear_acceleration(int i) { return (this->imu_data.linear_acceleration[i]); };

};

#endif
