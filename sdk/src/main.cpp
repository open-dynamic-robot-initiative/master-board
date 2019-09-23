/*
Etienne Arlaud
*/
#include <assert.h>
#include <unistd.h>

#include "ESPNOW_manager.h"
#include "ETHERNET_manager.h"
#include "protocol.h"

#include <chrono>
#include <math.h>
#include <defines.h>

#include <stdio.h>
#include <sys/stat.h>

#define PI 3.141592654
static uint8_t my_mac[6] = {0xa0, 0x1d, 0x48, 0x12, 0xa0, 0xc5};	 //{0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
static uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Broatcast to prevent acknoledgment behaviour

LINK_manager *handler;
uint8_t payload[127];

command_packet_t command_packet = {0};
sensor_packet_t sensor_packet = {0};
dual_motor_driver_sensor_data_t dual_motor_driver_sensor_data[N_SLAVES] = {0};

imu_data_t imu_data = {0};

#define MAX_HIST 20
int histogram_lost_sensor_packets[MAX_HIST]; //histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...
int histogram_lost_cmd_packets[MAX_HIST];		 //histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...

FILE *log_file;
int fd_log_file;

void print_hex_table(uint8_t *data, int len)
{
	for (int i = 0; i < len; i++)
		printf("%x ", data[i]);
}

void print_imu_data(imu_data_t data)
{
	printf("\nIMU:%8f %8f %8f %8f %8f %8f %8f %8f %8f",
				 data.accelerometer[0],
				 data.accelerometer[1],
				 data.accelerometer[2],
				 data.gyroscope[0],
				 data.gyroscope[1],
				 data.gyroscope[2],
				 data.attitude[0],
				 data.attitude[1],
				 data.attitude[2]);
}
void convert_raw_to_si_sensor_data(dual_motor_driver_sensor_packet_t raw, dual_motor_driver_sensor_data_t &si)
{
	si.is_system_enabled = raw.status & UD_SENSOR_STATUS_SE;
	si.is_motor_enabled[0] = raw.status & UD_SENSOR_STATUS_M1E;
	si.is_motor_enabled[1] = raw.status & UD_SENSOR_STATUS_M2E;
	si.is_motor_ready[0] = raw.status & UD_SENSOR_STATUS_M1R;
	si.is_motor_ready[1] = raw.status & UD_SENSOR_STATUS_M2R;
	si.has_index_been_detected[0] = raw.status & UD_SENSOR_STATUS_IDX1D;
	si.has_index_been_detected[1] = raw.status & UD_SENSOR_STATUS_IDX2D;
	si.index_toggle_bit[0] = raw.status & UD_SENSOR_STATUS_IDX1T;
	si.index_toggle_bit[1] = raw.status & UD_SENSOR_STATUS_IDX2T;
	si.error_code = raw.status & UD_SENSOR_STATUS_ERROR;
	si.position[0] = D32QN_TO_FLOAT(raw.position[0], UD_QN_POS);
	si.position[1] = D32QN_TO_FLOAT(raw.position[1], UD_QN_POS);
	si.velocity[0] = D32QN_TO_FLOAT(raw.velocity[0], UD_QN_VEL);
	si.velocity[1] = D32QN_TO_FLOAT(raw.velocity[1], UD_QN_VEL);
	si.current[0] = D32QN_TO_FLOAT(raw.current[0], UD_QN_IQ);
	si.current[1] = D32QN_TO_FLOAT(raw.current[1], UD_QN_IQ);
}

void convert_raw_to_si_imu_data(imu_packet_t raw, imu_data_t &si)
{ /* Qvalues for each fields */
	for (int i = 0; i < 3; i++)
	{
		si.accelerometer[i] = D16QN_TO_FLOAT(raw.accelerometer[i], IMU_QN_ACC);
		si.gyroscope[i] = D16QN_TO_FLOAT(raw.gyroscope[i], IMU_QN_GYR);
		si.attitude[i] = D16QN_TO_FLOAT(raw.attitude[i], IMU_QN_EF);
	}
}

uint16_t nb_recv = 0;
uint16_t last_sensor_index = 0;
uint32_t nb_sensors_sent = 0; //this variable deduce the total number of received sensor packet from sensor index and previous sensor index
uint32_t nb_sensors_lost = 0;

uint32_t nb_cmd_lost_offset = -1;
uint32_t last_cmd_lost = 0;

void callback(uint8_t src_mac[6], uint8_t *data, int len)
{
	if (len != 190)
	{
		printf("received a %d long packet\n", len);
		return;
	}
	memcpy(&sensor_packet, data, sizeof(sensor_packet_t));
	nb_recv++;
	for (int i = 0; i < N_SLAVES; i++)
	{
		convert_raw_to_si_sensor_data(sensor_packet.dual_motor_driver_sensor_packets[i], dual_motor_driver_sensor_data[i]);
	}
	convert_raw_to_si_imu_data(sensor_packet.imu, imu_data);
	if (last_sensor_index == 0)
	{
		last_sensor_index = sensor_packet.sensor_index - 1;
	}
	if (last_cmd_lost == 0)
	{
		last_cmd_lost = sensor_packet.last_index - 1;
	}
	if (nb_cmd_lost_offset > sensor_packet.last_index)
	{
		nb_cmd_lost_offset = sensor_packet.last_index;
	}

	//Check for sensor packet loss
	uint16_t actual_sensor_packets_loss = sensor_packet.sensor_index - last_sensor_index - 1;
	nb_sensors_lost += actual_sensor_packets_loss;
	nb_sensors_sent += actual_sensor_packets_loss + 1;
	last_sensor_index = sensor_packet.sensor_index;
	if (actual_sensor_packets_loss > 0)
	{
		if ((actual_sensor_packets_loss - 1) < MAX_HIST)
			histogram_lost_sensor_packets[actual_sensor_packets_loss]++;
		else
			histogram_lost_sensor_packets[MAX_HIST - 1]++;
	}

	//chech for cmd packet loss (This does'nt realy work. for debug sensor_packet.last_index field return the number of lost packet computed in the ESP32, and not the last command packet)
	int16_t actual_cmd_packets_loss = sensor_packet.last_index - last_cmd_lost - 1;
	last_cmd_lost = sensor_packet.last_index;

	if (actual_cmd_packets_loss > 0)
	{
		if ((actual_cmd_packets_loss - 1) < MAX_HIST)
			histogram_lost_cmd_packets[actual_cmd_packets_loss - 1]++;
		else
			histogram_lost_cmd_packets[MAX_HIST - 1]++;
	}
	if (nb_recv % 100 == 0)
	{
		printf("\e[1;1H\e[2J");
		for (int i = 0; i < N_SLAVES; i++)
		{
			printf("\n%d ", i);
			printf("err:%x ", dual_motor_driver_sensor_data[i].error_code);
			printf("SE:%d ", dual_motor_driver_sensor_data[i].is_system_enabled);
			printf("M1E:%d ", dual_motor_driver_sensor_data[i].is_motor_enabled[0]);
			printf("M2E:%d ", dual_motor_driver_sensor_data[i].is_motor_enabled[1]);
			printf("M1R:%d ", dual_motor_driver_sensor_data[i].is_motor_ready[0]);
			printf("M2R:%d ", dual_motor_driver_sensor_data[i].is_motor_ready[1]);
			printf("IDX1D:%d ", dual_motor_driver_sensor_data[i].has_index_been_detected[0]);
			printf("IDX2D:%d ", dual_motor_driver_sensor_data[i].has_index_been_detected[1]);
			printf("IDX1T:%d ", dual_motor_driver_sensor_data[i].index_toggle_bit[0]);
			printf("IDX2T:%d ", dual_motor_driver_sensor_data[i].index_toggle_bit[1]);
			printf("timestamp:%8x ", dual_motor_driver_sensor_data[i].timestamp);
			printf("pos1:%8f ", dual_motor_driver_sensor_data[i].position[0]);
			printf("pos2:%8f ", dual_motor_driver_sensor_data[i].position[1]);
			printf("vel1:%8f ", dual_motor_driver_sensor_data[i].velocity[0]);
			printf("vel2:%8f ", dual_motor_driver_sensor_data[i].velocity[1]);
			printf("cur1:%8f ", dual_motor_driver_sensor_data[i].current[0]);
			printf("cur2:%8f ", dual_motor_driver_sensor_data[i].current[1]);
		}
		print_imu_data(imu_data);
		printf("\n");
		printf("nb_sensors_sent: %u \n", nb_sensors_sent);
		printf("\n");
		printf("nb_sensors_lost: %u \n", nb_sensors_lost);
		printf("sensor ratio (lost): %2f\n ", 100.0 * nb_sensors_lost / nb_sensors_sent);
		printf("\n");
		printf("nb_cmd_lost: %u\n", sensor_packet.last_index - nb_cmd_lost_offset);
		printf("~cmd ratio (lost): %2f\n ", 100.0 * (sensor_packet.last_index - nb_cmd_lost_offset) / nb_sensors_sent);
		printf("\n");
		printf("\nPacket lost in groups of: \n");
		printf("       \t sensors \t commands \n");
		for (int i = 0; i < MAX_HIST; i++)
		{
			printf("  %3d : \t%d \t %d\n ", i + 1, histogram_lost_sensor_packets[i], histogram_lost_cmd_packets[i]);
		}
	}

	fflush(stdout);

	/*log imu*/
	if (log_file != NULL)
	{
		fprintf(log_file, "%8f, %8f, %8f, %8f, %8f\n",
						-imu_data.attitude[2],
						dual_motor_driver_sensor_data[0].position[1] * 2 * PI,
						-imu_data.gyroscope[2],
						dual_motor_driver_sensor_data[0].velocity[1] * (2 * PI / 60.0) * 1000.0,
						dual_motor_driver_sensor_data[0].position[0] * 2 * PI);
	}
}

int main(int argc, char **argv)
{
	assert(argc > 1);
	log_file = NULL;
	if (argc > 2)
	{
		log_file = fopen(argv[2], "w");
		//fprintf(log_file,"Ax, Ay, Az, Gx, Gy, Gz, R, P, Y\n");
		fprintf(log_file, "Yaw, PositionM2, GyrZ, VelocityM2, PositionM1\n");
		if (log_file == NULL)
		{
			printf("Could not write to '%s', no data will be saved! Continue? (Y/N)", argv[2]);
			char c = getchar();
			if (c == 'n' || c == 'N')
			{
				exit(1);
			}
		}
		else
		{
			chmod(argv[2], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
			printf("All data will be saved to '%s'", argv[2]);
		}
	}
	else
	{
		printf("No output file specified, data will not be saved");
	}
	nice(-20);
	printf("Payload size : %ld\n", sizeof(command_packet_t));

	if (argv[1][0] == 'e')
	{
		/*Ethernet*/
		printf("Using Ethernet (%s)", argv[1]);
		handler = new ETHERNET_manager(argv[1], my_mac, dest_mac);
		handler->set_recv_callback(&callback);
		handler->start();
	}
	else if (argv[1][0] == 'w')
	{
		/*WiFi*/
		printf("Using WiFi (%s)", argv[1]);
		handler = new ESPNOW_manager(argv[1], DATARATE_24Mbps, CHANNEL_freq_9, my_mac, dest_mac, false);
		((ESPNOW_manager *)handler)->set_filter(my_mac, dest_mac);
		handler->set_recv_callback(&callback);
		handler->start();
		((ESPNOW_manager *)handler)->bind_filter();
	}

	handler->set_dst_mac(dest_mac);
	int n_count = 0;
	std::chrono::time_point<std::chrono::system_clock> last;

	int state = 0;
	float pos_refA[N_SLAVES_CONTROLED] = {0};
	float pos_errA[N_SLAVES_CONTROLED] = {0};
	float vel_refA[N_SLAVES_CONTROLED] = {0};
	float vel_errA[N_SLAVES_CONTROLED] = {0};
	float iqA[N_SLAVES_CONTROLED] = {0};
	float pos_refB[N_SLAVES_CONTROLED] = {0};
	float pos_errB[N_SLAVES_CONTROLED] = {0};
	float vel_refB[N_SLAVES_CONTROLED] = {0};
	float vel_errB[N_SLAVES_CONTROLED] = {0};
	float iqB[N_SLAVES_CONTROLED] = {0};

	float Kp = 3.0;
	float Kd = 1.0;
	float iq_sat = 2.0;
	float freq = 1;
	float t = 0;
	while (1)
	{
		if (((std::chrono::duration<double>)(std::chrono::system_clock::now() - last)).count() > 0.001)
		{

			last = std::chrono::system_clock::now();

			t += 0.001;
			switch (state)
			{
			case 0:
				//Initialisation, send the init commands
				for (int i = 0; i < N_SLAVES_CONTROLED; i++)
				{
					SUB_REG_u16(command_packet.command[i], UD_COMMAND_MODE) = UD_COMMAND_MODE_ES | UD_COMMAND_MODE_EM1 | UD_COMMAND_MODE_EM2; // | UD_COMMAND_MODE_EI1OC | UD_COMMAND_MODE_EI2OC
				}
				//check the end of calibration (are the all ontrolled motor ready?)
				state = 1;
				for (int i = 0; i < N_SLAVES_CONTROLED; i++)
				{
					if (!(dual_motor_driver_sensor_data[i].is_motor_enabled[0] && dual_motor_driver_sensor_data[i].is_motor_enabled[1]))
					{
						state = 0; //calibration is not finished
					}
				}
				break;
			case 1:
				//closed loop, position
				for (int i = 0; i < N_SLAVES_CONTROLED; i++)
				{
					if (dual_motor_driver_sensor_data[i].is_system_enabled)
					{
						pos_refA[i] = sin(2 * PI * freq * t);
						pos_errA[i] = pos_refA[i] - dual_motor_driver_sensor_data[i].position[0];
						vel_errA[i] = vel_refA[i] - dual_motor_driver_sensor_data[i].velocity[0];
						iqA[i] = Kp * pos_errA[i] + Kd * vel_errA[i];
						if (iqA[i] > iq_sat)
							iqA[i] = iq_sat;
						if (iqA[i] < -iq_sat)
							iqA[i] = -iq_sat;

						pos_refB[i] = sin(2 * PI * freq * t);
						pos_errB[i] = pos_refB[i] - dual_motor_driver_sensor_data[i].position[1];
						vel_errB[i] = vel_refB[i] - dual_motor_driver_sensor_data[i].velocity[1];
						iqB[i] = Kp * pos_errB[i] + Kd * vel_errB[i];
						if (iqB[i] > iq_sat)
							iqB[i] = iq_sat;
						if (iqB[i] < -iq_sat)
							iqB[i] = -iq_sat;

						SUB_REG_u16(command_packet.command[i], UD_COMMAND_MODE) = UD_COMMAND_MODE_ES | UD_COMMAND_MODE_EM1 | UD_COMMAND_MODE_EM2; //| UD_COMMAND_MODE_EI1OC | UD_COMMAND_MODE_EI2OC
						SUB_REG_16(command_packet.command[i], UD_COMMAND_IQ_1) = FLOAT_TO_D16QN(iqA[i], UD_QN_IQ);
						SUB_REG_16(command_packet.command[i], UD_COMMAND_IQ_2) = FLOAT_TO_D16QN(iqB[i], UD_QN_IQ);
						command_packet.command[i][UD_COMMAND_MODE] &= 0xff00; //Set timeout to 0
						command_packet.command[i][UD_COMMAND_MODE] |= 0x00ff; //Set timeout to 10
					}
					else
					{
						printf("E%d ", i);
					}
				}
				break;
			}
			command_packet.sensor_index++;
			handler->send((uint8_t *)&command_packet, sizeof(command_packet_t)),
					n_count++;
		}
		else
		{
			std::this_thread::yield();
		}
	}
	if (log_file != NULL)
	{
		fclose(log_file);
	}
	handler->end();
}
