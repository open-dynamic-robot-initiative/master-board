/*
Etienne Arlaud
*/
#include <assert.h>
#include <unistd.h>

#include "ESPNOW_manager.h"
#include "ETHERNET_manager.h"
#include "spi_quad_packet.h"

#include <chrono>
#include <math.h>
#include <defines.h>

static uint8_t my_mac[6] = {0xa0, 0x1d, 0x48, 0x12, 0xa0, 0xc5};   //{0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
static uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //Broatcast to prevent acknoledgment behaviour

LINK_manager *handler;
uint8_t payload[127];

wifi_eth_packet_command my_command = {0};
wifi_eth_packet_sensor raw_sensor_packet = {0};
si_sensor_data uDrivers_si_sensor_data[N_SLAVES]  = {0};
#define MAX_HIST 20
int histogram_lost_sensor_packets[MAX_HIST]; //histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...
int histogram_lost_cmd_packets[MAX_HIST];    //histogram_lost_packets[0] is the number of single packet loss, histogram_lost_packets[1] is the number of two consecutive packet loss, etc...



void print_hex_table(uint8_t * data,int len)
{
	for (int i=0; i<len;i++)
		printf("%x ",data[i]);
}

void convert_raw_to_si_sensor_data(raw_sensor_data raw,si_sensor_data &si)
{
    si.is_system_enabled   = raw.status & SPI_SENSOR_STATUS_SE;
    si.is_motor_enabled[0] = raw.status & SPI_SENSOR_STATUS_M1E;
    si.is_motor_enabled[1] = raw.status & SPI_SENSOR_STATUS_M2E;
    si.is_motor_ready[0]   = raw.status & SPI_SENSOR_STATUS_M1R;
    si.is_motor_ready[1]   = raw.status & SPI_SENSOR_STATUS_M2R;
	si.error_code          = raw.status & SPI_SENSOR_STATUS_ERROR ;
	si.position[0]=D32QN_TO_FLOAT(raw.position[0],SPI_QN_POS);
	si.position[1]=D32QN_TO_FLOAT(raw.position[1],SPI_QN_POS);
	si.velocity[0]=D32QN_TO_FLOAT(raw.velocity[0],SPI_QN_VEL);
	si.velocity[1]=D32QN_TO_FLOAT(raw.velocity[1],SPI_QN_VEL);
	si.current[0]=D32QN_TO_FLOAT(raw.current[0],SPI_QN_IQ);
	si.current[1]=D32QN_TO_FLOAT(raw.current[1],SPI_QN_IQ);
}

uint16_t nb_recv = 0;
uint16_t last_sensor_index = 0;
uint32_t nb_sensors_sent = 0; //this variable deduce the total number of received sensor packet from sensot index and previous sensor index
uint32_t nb_sensors_lost = 0; 

uint32_t nb_cmd_lost_offset = -1;
uint32_t last_cmd_lost = 0; 

void callback(uint8_t src_mac[6], uint8_t *data, int len) {
	if (len!=190) 
	{
		printf("received a %d long packet\n",len);
		return;
	}
	memcpy(&raw_sensor_packet,data,sizeof(wifi_eth_packet_sensor));
	nb_recv++;
	for(int i=0; i<N_SLAVES;i++)
	{
		convert_raw_to_si_sensor_data(raw_sensor_packet.raw_uDriver_sensor_data[i],uDrivers_si_sensor_data[i]);
	}
	if(last_sensor_index == 0) 
	{
		last_sensor_index = raw_sensor_packet.sensor_index -1;
	}
	if(last_cmd_lost == 0) {
		last_cmd_lost = raw_sensor_packet.last_index - 1;
	}
	if(nb_cmd_lost_offset > raw_sensor_packet.last_index) {
		nb_cmd_lost_offset = raw_sensor_packet.last_index;
	}

	//Check for sensor packet loss
	uint16_t actual_sensor_packets_loss = raw_sensor_packet.sensor_index - last_sensor_index - 1;
	nb_sensors_lost+=actual_sensor_packets_loss;
	nb_sensors_sent+=actual_sensor_packets_loss+1;
	last_sensor_index = raw_sensor_packet.sensor_index;
	if (actual_sensor_packets_loss>0)
	{
		if ((actual_sensor_packets_loss-1)<MAX_HIST)
			histogram_lost_sensor_packets[actual_sensor_packets_loss]++;
		else
			histogram_lost_sensor_packets[MAX_HIST-1]++;
	}	
	
	
	//chech for cmd packet loss (This does'nt realy work. for debug raw_sensor_packet.last_index field return the number of lost packet computed in the ESP32, and not the last command packet)
	int16_t actual_cmd_packets_loss = raw_sensor_packet.last_index - last_cmd_lost - 1;
	last_cmd_lost = raw_sensor_packet.last_index;

	if (actual_cmd_packets_loss>0)
	{
		if ((actual_cmd_packets_loss-1)<MAX_HIST)
			histogram_lost_cmd_packets[actual_cmd_packets_loss-1]++;
		else
			histogram_lost_cmd_packets[MAX_HIST-1]++;
	}
	if (nb_recv%100 == 0)
	{
		printf("\e[1;1H\e[2J");
		for(int i=0; i<N_SLAVES;i++)
		{
			printf("\n%d ",i);
			printf("err:%x ",uDrivers_si_sensor_data[i].error_code);
			printf("SE:%d ",uDrivers_si_sensor_data[i].is_system_enabled);
			printf("M1E:%d ",uDrivers_si_sensor_data[i].is_motor_enabled[0]);
			printf("M2E:%d ",uDrivers_si_sensor_data[i].is_motor_enabled[1]);
			printf("M1R:%d ",uDrivers_si_sensor_data[i].is_motor_ready[0]);
			printf("M2R:%d ",uDrivers_si_sensor_data[i].is_motor_ready[1]);
			
			printf("timestamp:%8x ",uDrivers_si_sensor_data[i].timestamp);
			printf("pos1:%8f ",uDrivers_si_sensor_data[i].position[0]);
			printf("pos2:%8f ",uDrivers_si_sensor_data[i].position[1]);
			printf("vel1:%8f ",uDrivers_si_sensor_data[i].velocity[0]);
			printf("vel2:%8f ",uDrivers_si_sensor_data[i].velocity[1]);
			printf("cur1:%8f ",uDrivers_si_sensor_data[i].current[0]);
			printf("cur2:%8f ",uDrivers_si_sensor_data[i].current[1]);
			
			
		}
		printf("\n");
		printf("nb_sensors_sent: %u \n",nb_sensors_sent);
		printf("\n");
		printf("nb_sensors_lost: %u \n",nb_sensors_lost);
		printf("sensor ratio (lost): %2f\n ",100.0*nb_sensors_lost/nb_sensors_sent);
		printf("\n");
		printf("nb_cmd_lost: %u\n",raw_sensor_packet.last_index - nb_cmd_lost_offset);
		printf("~cmd ratio (lost): %2f\n ",100.0*(raw_sensor_packet.last_index - nb_cmd_lost_offset)/nb_sensors_sent);
		printf("\n");
		printf("\nPacket lost in groups of: \n");
		printf("       \t sensors \t commands \n");
		for(int i=0;i<MAX_HIST;i++)
		{
			printf("  %3d : \t%d \t %d\n ",i+1,histogram_lost_sensor_packets[i],histogram_lost_cmd_packets[i]);
		}
	}
	
	fflush(stdout);
}

int main(int argc, char **argv) {
	assert(argc > 1);
	nice(-20);
	argv[1];
	printf("Payload size : %ld\n", sizeof(wifi_eth_packet_command));
	
	if (argv[1][0]=='e')
	{
	/*Ethernet*/
		printf("Using Ethernet (%s)",argv[1]);
		handler = new ETHERNET_manager(argv[1], my_mac, dest_mac);
		handler->set_recv_callback(&callback);
		handler->start();
	}
	else if (argv[1][0]=='w')
	{
	/*WiFi*/
		printf("Using WiFi (%s)",argv[1]);
		handler = new ESPNOW_manager(argv[1], DATARATE_24Mbps, CHANNEL_freq_9, my_mac, dest_mac, false);
		((ESPNOW_manager *) handler)->set_filter(my_mac, dest_mac);
		handler->set_recv_callback(&callback);
		handler->start();
		((ESPNOW_manager *) handler)->bind_filter();
	}

	handler->set_dst_mac(dest_mac);
	int n_count = 0;
	std::chrono::time_point<std::chrono::system_clock> last;

	int state = 0;
	float pos_refA[N_SLAVES_CONTROLED]={0};
	float pos_errA[N_SLAVES_CONTROLED]={0};
	float vel_refA[N_SLAVES_CONTROLED]={0};
	float vel_errA[N_SLAVES_CONTROLED]={0};
	float iqA[N_SLAVES_CONTROLED]={0};
	float pos_refB[N_SLAVES_CONTROLED]={0};
	float pos_errB[N_SLAVES_CONTROLED]={0};
	float vel_refB[N_SLAVES_CONTROLED]={0};
	float vel_errB[N_SLAVES_CONTROLED]={0};
	float iqB[N_SLAVES_CONTROLED]={0};
	
	float Kp = 3.0;
	float Kd = 1.0;
	float iq_sat = 2.0;
	float PI=3.14;//todo find PI
	float freq = 1;
	float t=0;
	while(1) {
		if(((std::chrono::duration<double>) (std::chrono::system_clock::now() - last)).count() > 0.001) {

			last = std::chrono::system_clock::now();

			
			t +=0.001;
			switch (state)
			{
				case 0:
					//Initialisation, send the init commands
					for(int i=0; i<N_SLAVES_CONTROLED; i++)
					{
						SPI_REG_u16(my_command.command[i], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2 | SPI_COMMAND_MODE_CALIBRATE_M1 | SPI_COMMAND_MODE_CALIBRATE_M2;
					}
					//check the end of calibration (are the all ontrolled motor ready?)
					state = 1;
					for(int i=0; i<N_SLAVES_CONTROLED; i++)
					{
						if (!(uDrivers_si_sensor_data[i].is_motor_enabled[0] && uDrivers_si_sensor_data[i].is_motor_enabled[1]))
						{
							state = 0; //calibration is not finished
						}
					}
					break;
				case 1:
					//closed loop, position
					for(int i=0; i<N_SLAVES_CONTROLED; i++)
					{
						if (uDrivers_si_sensor_data[i].is_system_enabled)
						{
							pos_refA[i] = sin(2*PI*freq*t);
							pos_errA[i] = pos_refA[i] - uDrivers_si_sensor_data[i].position[0];
							vel_errA[i] = vel_refA[i] - uDrivers_si_sensor_data[i].velocity[0];
							iqA[i] = Kp * pos_errA[i] + Kd * vel_errA[i];
							if (iqA[i] >  iq_sat) iqA[i]= iq_sat;
							if (iqA[i] < -iq_sat) iqA[i]=-iq_sat;
							
							pos_refB[i] = sin(2*PI*freq*t);
							pos_errB[i] = pos_refB[i] - uDrivers_si_sensor_data[i].position[1];
							vel_errB[i] = vel_refB[i] - uDrivers_si_sensor_data[i].velocity[1];
							iqB[i] = Kp * pos_errB[i] + Kd * vel_errB[i];
							if (iqB[i] >  iq_sat) iqB[i]= iq_sat;
							if (iqB[i] < -iq_sat) iqB[i]=-iq_sat;				
							

							SPI_REG_u16(my_command.command[i], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2;
							SPI_REG_16(my_command.command[i], SPI_COMMAND_IQ_1) = FLOAT_TO_D16QN(iqA[i], SPI_QN_IQ);
							SPI_REG_16(my_command.command[i], SPI_COMMAND_IQ_2) = FLOAT_TO_D16QN(iqB[i], SPI_QN_IQ);
							my_command.command[i][SPI_COMMAND_MODE]&=0xff00; //Set timeout to 0
							my_command.command[i][SPI_COMMAND_MODE]|=0x00ff; //Set timeout to 10
						}
						else
						{
						    printf("E%d ",i);
						}
					}
					break;
			}
			my_command.sensor_index++;
			handler->send((uint8_t *) &my_command, sizeof(wifi_eth_packet_command)),
			n_count++;

		} else {
			std::this_thread::yield();
		}
	}

	handler->end();
}
