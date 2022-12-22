#include <assert.h>
#include <unistd.h>
#include <chrono>
#include <math.h>
#include <stdio.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>

#include "master_board_sdk/master_board_interface.h"
#include "master_board_sdk/defines.h"

#define LOG_STEPS 10000

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		throw std::runtime_error("Please provide the interface name "
								 "(i.e. using 'ifconfig' on linux");
	}

	int cpt = 0;
	double dt = 0.001;
	imu_data_t imu_logs[LOG_STEPS];

	nice(-20); //give the process a high priority
	printf("-- Main --\n");
	MasterBoardInterface robot_if(argv[1]);
	robot_if.Init();

	std::chrono::time_point<std::chrono::system_clock> last = std::chrono::system_clock::now();
	while (!robot_if.IsTimeout() && !robot_if.IsAckMsgReceived()) {
		if (((std::chrono::duration<double>)(std::chrono::system_clock::now() - last)).count() > dt)
		{
			last = std::chrono::system_clock::now();
			robot_if.SendInit();
		}
	}

	if (robot_if.IsTimeout())
	{
		printf("Timeout while waiting for ack.\n");
	}

	while (!robot_if.IsTimeout() && cpt < LOG_STEPS)
	{
		if (((std::chrono::duration<double>)(std::chrono::system_clock::now() - last)).count() > dt)
		{
			last = std::chrono::system_clock::now(); //last+dt would be better
			robot_if.ParseSensorData(); // This will read the last incomming packet and update all sensor fields.
			
			imu_logs[cpt % LOG_STEPS] = robot_if.get_imu_data();
			
			if (cpt % 100 == 0)
			{
				printf("\33[H\33[2J"); //clear screen
				robot_if.PrintIMU();
				robot_if.PrintStats();
				fflush(stdout);
				 

			}
			robot_if.SendCommand(); //This will send the command packet
			cpt++;
		}
		else
		{
			std::this_thread::yield();
		}
	}

	if (robot_if.IsTimeout()) {
		printf("Masterboard timeout detected. Either the masterboard has been shut down or there has been a connection issue with the cable/wifi.\n");
		return -1;
	}

	// If we reach here, we want to log the last 10 seconds of IMU data.

	std::ofstream myfile;
	myfile.open ("imu_data_collection.csv");
	for (int i = 0; i < LOG_STEPS; i++) 
	{
		myfile << imu_logs[i].accelerometer[0] << ","
			   << imu_logs[i].accelerometer[1] << ","
			   << imu_logs[i].accelerometer[2] << ","
			   << imu_logs[i].gyroscope[0] << ","
			   << imu_logs[i].gyroscope[1] << ","
			   << imu_logs[i].gyroscope[2] << ","
			   << imu_logs[i].attitude[0] << ","
			   << imu_logs[i].attitude[1] << ","
			   << imu_logs[i].attitude[2] << ","
			   << imu_logs[i].linear_acceleration[0] << ","
			   << imu_logs[i].linear_acceleration[1] << ","
			   << imu_logs[i].linear_acceleration[2] << "\n";
	}
	
	myfile.close();
	return 0;
}
