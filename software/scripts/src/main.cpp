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



static uint8_t my_mac[6] = {0xa0, 0x1d, 0x48, 0x12, 0xa0, 0xc5};//{0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
static uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t ESP_mac[6] = {0xb4, 0xe6, 0x2d, 0xb5, 0x9f, 0x85}; //{0xcc,0x50,0xe3,0xB6,0xb4,0x58};

LINK_manager *handler;
uint8_t payload[127];



struct wifi_eth_packet_command {
    uint16_t command[N_SLAVES][SPI_TOTAL_INDEX];
    uint16_t sensor_index;
}__attribute__ ((packed)) my_command;


struct raw_sensor_data {
uint16_t status;
uint16_t timestamp;
int32_t position[2];
int16_t velocity[2];
int16_t current[2];
uint16_t coil_resistance[2];
uint16_t adc[2];
}__attribute__ ((packed));

struct si_sensor_data {
	uint16_t status;
	uint16_t timestamp;// to be removed
	bool is_system_enabled; 
	bool is_motor_enabled[2];
	bool is_motor_ready[2];
	uint8_t error_code;  
	float position[2];
	float velocity[2];
	float current[2];
	float coil_resistance[2];
	float adc[2];

}__attribute__ ((packed));



struct wifi_eth_packet_sensor {
    struct raw_sensor_data raw_uDriver_sensor_data[N_SLAVES];
    uint8_t IMU[18]; //TODO create the appropriate struct
    uint16_t sensor_index;
    uint16_t last_index;
} __attribute__ ((packed));


wifi_eth_packet_sensor raw_sensor_packet = {0};
si_sensor_data  uDrivers_si_sensor_data[N_SLAVES]  = {0};


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

void callback(uint8_t src_mac[6], uint8_t *data, int len) {
	if (len!=190) 
	{
		printf("received a %d long packet\n",len);
		return;
	}
	//~ memcpy(uDrivers_raw_sensor_data,data,sizeof(raw_sensor_data)*N_SLAVES);
	memcpy(&raw_sensor_packet,data,sizeof(wifi_eth_packet_sensor));
	nb_recv++;
	/*
	 * //echo
	handler->mypacket->set_payload_len(127 + 5);
	memcpy(handler->mypacket->get_payload_ptr(), data, 6);
	//handler->set_dst_mac(dest_mac);
	handler->send();
	*/
	//print_hex_table(data,len);
	for(int i=0; i<N_SLAVES;i++)
	{
		convert_raw_to_si_sensor_data(raw_sensor_packet.raw_uDriver_sensor_data[i],uDrivers_si_sensor_data[i]);
	}
	if(last_sensor_index == 0) 
	{
		last_sensor_index = raw_sensor_packet.sensor_index -1;
	}
	//Check for packet loss
	uint16_t sensor_packets_loss = raw_sensor_packet.sensor_index - last_sensor_index - (uint16_t)1;
	
	nb_sensors_lost+=sensor_packets_loss;
	nb_sensors_sent+=sensor_packets_loss+1;
	last_sensor_index = raw_sensor_packet.sensor_index;
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
		printf("nb_sensors_sent: %lu \n ",nb_sensors_sent);
		printf("nb_sensors_lost: %lu \n",nb_sensors_lost);
		printf("sensor-packet-lost: %lu \n",sensor_packets_loss);
		printf("ratio: %4f\n ",100.0*nb_sensors_lost/nb_sensors_sent);
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
	    printf("Using Ethenet (%s)\n", sizeof(wifi_eth_packet_command));
		handler = new ETHERNET_manager(argv[1], my_mac, dest_mac);
		handler->set_recv_callback(&callback);
		handler->start();
	}
	else if (argv[1][0]=='w')
	{
	/*WiFi*/
		handler = new ESPNOW_manager(argv[1], DATARATE_24Mbps, CHANNEL_freq_9, my_mac, dest_mac, false);
		((ESPNOW_manager *) handler)->set_filter(my_mac, dest_mac);
		handler->set_recv_callback(&callback);
		handler->start();
		((ESPNOW_manager *) handler)->bind_filter();
	}
	printf("Payload size : %ld\n", sizeof(wifi_eth_packet_command));
	memset(&my_command, 0, sizeof(wifi_eth_packet_command));

	handler->set_dst_mac(dest_mac);
	
	int n_count = 0;
	std::chrono::time_point<std::chrono::system_clock> last;


	int state = 0;
	int slave_idx = 1;
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
	
	float Kp = 10.0;
	float Kd = 4.0;
	float iq_sat = 2.0;
	float PI=3.14;//todo find PI
	float freq = 1;
	float t=0;
	float mean_pos = 0;
	while(1) {
		if(((std::chrono::duration<double>) (std::chrono::system_clock::now() - last)).count() > 0.001) {

			last = std::chrono::system_clock::now();
			my_command.sensor_index++;
			handler->send((uint8_t *) &my_command, sizeof(wifi_eth_packet_command)),
			n_count++;
			
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
						if (!(uDrivers_si_sensor_data[slave_idx].is_motor_enabled[0] && uDrivers_si_sensor_data[slave_idx].is_motor_enabled[1]))
						{
							state = 0; //calibration is not finished
						}
					}
					break;
				case 1:
					//Command the motor!
					//open loop, constant current
					//~ SPI_REG_u16(my_command.command[slave_idx], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2;
					//~ SPI_REG_16(my_command.command[slave_idx], SPI_COMMAND_IQ_1) = FLOAT_TO_D16QN(0.1, SPI_QN_IQ);
					//~ SPI_REG_16(my_command.command[slave_idx], SPI_COMMAND_IQ_2) = FLOAT_TO_D16QN(-0.1, SPI_QN_IQ);
					
					
					int n_mean_pos = 0;
					
					//closed loop, position
					for(int i=0; i<N_SLAVES_CONTROLED; i++)
					{
						if (uDrivers_si_sensor_data[i].is_system_enabled)
						{
							//~ pos_refA[i] = sin(2*PI*freq*t);
							if (i==0) mean_pos = uDrivers_si_sensor_data[i].position[0];
							pos_refA[i] = mean_pos;
							pos_errA[i] = pos_refA[i] - uDrivers_si_sensor_data[i].position[0];
							vel_errA[i] = vel_refA[i] - uDrivers_si_sensor_data[i].velocity[0];
							iqA[i] = Kp * pos_errA[i] + Kd * vel_errA[i];
							if (iqA[i] >  iq_sat) iqA[i]= iq_sat;
							if (iqA[i] < -iq_sat) iqA[i]=-iq_sat;
							
							if (i==0) iqA[i] = 0.;//todo rem
							
							
							//~ pos_refB[i] = sin(2*PI*freq*t);
							pos_refB[i] = mean_pos;
							pos_errB[i] = pos_refB[i] - uDrivers_si_sensor_data[i].position[1];
							vel_errB[i] = vel_refB[i] - uDrivers_si_sensor_data[i].velocity[1];
							iqB[i] = Kp * pos_errB[i] + Kd * vel_errB[i];
							if (iqB[i] >  iq_sat) iqB[i]= iq_sat;
							if (iqB[i] < -iq_sat) iqB[i]=-iq_sat;				
							
							SPI_REG_u16(my_command.command[i], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2;
							SPI_REG_16(my_command.command[i], SPI_COMMAND_IQ_1) = FLOAT_TO_D16QN(iqA[i], SPI_QN_IQ);
							SPI_REG_16(my_command.command[i], SPI_COMMAND_IQ_2) = FLOAT_TO_D16QN(iqB[i], SPI_QN_IQ);
							//~ mean_pos+=uDrivers_si_sensor_data[i].position[0];
							//~ mean_pos+=uDrivers_si_sensor_data[i].position[1];
							//~ n_mean_pos+=2;
						}
						else
						{
						    printf("E%d ",i);
						}
					}
					//~ if (n_mean_pos>0) mean_pos = mean_pos / (n_mean_pos + 1);
					//~ printf("mean_pos %lf ",mean_pos);
					break;
			}

		} else {
			std::this_thread::yield();
		}
	}

	handler->end();
}
