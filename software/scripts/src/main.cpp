/*
Etienne Arlaud
*/
#include <assert.h>
#include <unistd.h>

#include "ESPNOW_manager.h"
#include "ETHERNET_manager.h"
#include "spi_quad_packet.h"

#include <chrono>

#define N_SLAVES 6

#define D32Q24_TO_D16QN(a,n)      ((a)>>(24-(n))&0xFFFF)
#define D32Q24_TO_D8QN(a,n)       ((a)>>(24-(n))&0xFF)

#define uD16QN_TO_D32Q24(a,n)      (((uint32_t)(a))<<(24-(n)))
#define uD8QN_TO_D32Q24(a,n)       (((uint32_t)(a))<<(24-(n)))

#define D16QN_TO_D32Q24(a,n)      (((int32_t)(a))<<(24-(n)))
#define D8QN_TO_D32Q24(a,n)       (((int32_t)(a))<<(24-(n)))

#define D32QN_TO_FLOAT(a,n)       ((float)(a)) / (1<<(n))
#define D16QN_TO_FLOAT(a,n)       ((float)(a)) / (1<<(n))
#define D8QN_TO_FLOAT(a,n)        ((float)(a)) / (1<<(n))

#define FLOAT_TO_uD32QN(a,n)      ((uint32_t) ((a) * (1<<(n))))
#define FLOAT_TO_uD16QN(a,n)      ((uint16_t) ((a) * (1<<(n))))
#define FLOAT_TO_uD8QN (a,n)      ((uint8_t)  ((a) * (1<<(n))))

#define FLOAT_TO_D32QN(a,n)       ((int32_t) ((a) * (1<<(n))))
#define FLOAT_TO_D16QN(a,n)       ((int16_t) ((a) * (1<<(n))))
#define FLOAT_TO_D8QN (a,n)       ((int8_t)  ((a) * (1<<(n))))

/* Qvalues for each fields */
#define SPI_QN_POS  24
#define SPI_QN_VEL  11
#define SPI_QN_IQ   10
#define SPI_QN_ISAT 3
#define SPI_QN_CR   15
#define SPI_QN_ADC  16
#define SPI_QN_KP   16
#define SPI_QN_KD   16

static uint8_t my_mac[6] = {0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
static uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t ESP_mac[6] = {0xb4, 0xe6, 0x2d, 0xb5, 0x9f, 0x85}; //{0xcc,0x50,0xe3,0xB6,0xb4,0x58};

LINK_manager *handler;
uint8_t payload[127];



struct wifi_eth_packet_command {
    uint16_t command[6][SPI_TOTAL_INDEX];
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
	uint16_t timestamp;
	float position[2];
	float velocity[2];
	float current[2];
	float coil_resistance[2];
	float adc[2];
}__attribute__ ((packed));


raw_sensor_data uDrivers_raw_sensor_data[N_SLAVES] = {0};
si_sensor_data  uDrivers_si_sensor_data[N_SLAVES]  = {0};

void print_hex_table(uint8_t * data,int len)
{
	for (int i=0; i<len;i++)
		printf("%x ",data[i]);
}

void convert_raw_to_si_sensor_data(raw_sensor_data raw,si_sensor_data &si)
{
	si.position[0]=D32QN_TO_FLOAT(raw.position[0],SPI_QN_POS);
	si.position[1]=D32QN_TO_FLOAT(raw.position[1],SPI_QN_POS);
	si.velocity[0]=D32QN_TO_FLOAT(raw.velocity[0],SPI_QN_VEL);
	si.velocity[1]=D32QN_TO_FLOAT(raw.velocity[1],SPI_QN_VEL);
	si.current[0]=D32QN_TO_FLOAT(raw.current[0],SPI_QN_IQ);
	si.current[1]=D32QN_TO_FLOAT(raw.current[1],SPI_QN_IQ);
	
}

uint16_t nb_recv = 0;

void callback(uint8_t src_mac[6], uint8_t *data, int len) {

	if (len!=190) 
	{
		
		printf("received a %d long packet\n",len);
		return;
	}
	memcpy(uDrivers_raw_sensor_data,data,sizeof(raw_sensor_data)*N_SLAVES);
	nb_recv++;
	/*
	 * //echo
	handler->mypacket->set_payload_len(127 + 5);
	memcpy(handler->mypacket->get_payload_ptr(), data, 6);
	//handler->set_dst_mac(dest_mac);
	handler->send();
	*/
	//print_hex_table(data,len);
	if (nb_recv%100 == 1)
	{
		printf("\e[1;1H\e[2J");
		for(int i=0; i<N_SLAVES;i++)
		{
			printf("\n%d ",i);
			printf("status:%8x ",uDrivers_raw_sensor_data[i].status);
			printf("timestamp:%8x ",uDrivers_raw_sensor_data[i].timestamp);
			printf("posA:%8x ",uDrivers_raw_sensor_data[i].position[0]);
			printf("posB:%8x ",uDrivers_raw_sensor_data[i].position[1]);
			printf("velA:%8x ",uDrivers_raw_sensor_data[i].velocity[0]);
			printf("velB:%8x ",uDrivers_raw_sensor_data[i].velocity[1]);
			printf("curA:%8x ",uDrivers_raw_sensor_data[i].current[0]);
			printf("curB:%8x ",uDrivers_raw_sensor_data[i].current[1]);	    
			convert_raw_to_si_sensor_data(uDrivers_raw_sensor_data[i],uDrivers_si_sensor_data[i]);
		}
		printf("\n");
		for(int i=0; i<N_SLAVES;i++)
		{
			printf("\n%d ",i);
			printf("status:%8x ",uDrivers_si_sensor_data[i].status);
			printf("timestamp:%8x ",uDrivers_si_sensor_data[i].timestamp);
			printf("posA:%8f ",uDrivers_si_sensor_data[i].position[0]);
			printf("posB:%8f ",uDrivers_si_sensor_data[i].position[1]);
			printf("velA:%8f ",uDrivers_si_sensor_data[i].velocity[0]);
			printf("velB:%8f ",uDrivers_si_sensor_data[i].velocity[1]);
			printf("curA:%8f ",uDrivers_si_sensor_data[i].current[0]);
			printf("curB:%8f ",uDrivers_si_sensor_data[i].current[1]);
			
		
		}
		printf("\n");
	}
	fflush(stdout);
}

int main(int argc, char **argv) {
	assert(argc > 1);

	nice(-20);

	handler = new ETHERNET_manager(argv[1], my_mac, dest_mac);
	//handler = new ESPNOW_manager(argv[1], DATARATE_24Mbps, CHANNEL_freq_9, my_mac, dest_mac, false);

	//((ESPNOW_manager *) handler)->set_filter(my_mac, dest_mac);

	handler->set_recv_callback(&callback);

	handler->start();

	//((ESPNOW_manager *) handler)->bind_filter();

	printf("Payload size : %ld\n", sizeof(wifi_eth_packet_command));
	memset(&my_command, 0, sizeof(wifi_eth_packet_command));

	handler->set_dst_mac(dest_mac);
	
	int n_count = 0;
	std::chrono::time_point<std::chrono::system_clock> last;

	while(1) {
		if(((std::chrono::duration<double>) (std::chrono::system_clock::now() - last)).count() > 0.001) {
			if(n_count < 100) {
				SPI_REG_u16(my_command.command[1], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2 | SPI_COMMAND_MODE_CALIBRATE_M1 | SPI_COMMAND_MODE_CALIBRATE_M2;

				SPI_REG_u16(my_command.command[2], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2 | SPI_COMMAND_MODE_CALIBRATE_M1 | SPI_COMMAND_MODE_CALIBRATE_M2;
			} else {
				SPI_REG_u16(my_command.command[1], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2;
				SPI_REG_16(my_command.command[1], SPI_COMMAND_IQ_1) = FLOAT_TO_D16QN(0.1, SPI_QN_IQ);
				SPI_REG_16(my_command.command[1], SPI_COMMAND_IQ_2) = FLOAT_TO_D16QN(-0.1, SPI_QN_IQ);
				
				SPI_REG_u16(my_command.command[2], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2;
				SPI_REG_16(my_command.command[2], SPI_COMMAND_IQ_1) = FLOAT_TO_D16QN(0.1, SPI_QN_IQ);
				SPI_REG_16(my_command.command[2], SPI_COMMAND_IQ_2) = FLOAT_TO_D16QN(-0.1, SPI_QN_IQ);
			}
			
			last = std::chrono::system_clock::now();

			my_command.sensor_index++;

			handler->send((uint8_t *) &my_command, sizeof(wifi_eth_packet_command)),

			n_count++;
		} else {
			std::this_thread::yield();
		}
	}

	handler->end();
}
