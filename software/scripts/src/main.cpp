/*
Etienne Arlaud
*/
#include <assert.h>
#include <unistd.h>

#include "ESPNOW_manager.h"
#include "ETHERNET_manager.h"
#include "spi_quad_packet.h"

#include <chrono>

static uint8_t my_mac[6] = {0xF8, 0x1A, 0x67, 0xb7, 0xEB, 0x0B};
static uint8_t dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t ESP_mac[6] = {0xb4, 0xe6, 0x2d, 0xb5, 0x9f, 0x85}; //{0xcc,0x50,0xe3,0xB6,0xb4,0x58};

LINK_manager *handler;

uint8_t payload[127];


struct wifi_eth_packet_command {
    uint16_t command[6][SPI_TOTAL_INDEX];
    uint16_t sensor_index;
}__attribute__ ((packed)) my_command;

void callback(uint8_t src_mac[6], uint8_t *data, int len) {
	/*
	handler->mypacket->set_payload_len(127 + 5);
	memcpy(handler->mypacket->get_payload_ptr(), data, 6);
	//handler->set_dst_mac(dest_mac);
	handler->send();
	*/
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
			if(n_count < 10) {
				spi_get16(my_command.command[1], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2 | SPI_COMMAND_MODE_CALIBRATE_M1 | SPI_COMMAND_MODE_CALIBRATE_M2;
			} else {
				spi_get16(my_command.command[1], SPI_COMMAND_MODE) = SPI_COMMAND_MODE_ES | SPI_COMMAND_MODE_EM1 | SPI_COMMAND_MODE_EM2;

				spi_get16(my_command.command[1], SPI_COMMAND_IQ_1) = FLOAT_TO_D16QN(0.1, SPI_QN_IQ);
				//TODO: Negative float values don't work -> motor spins way to fast
				spi_get16(my_command.command[1], SPI_COMMAND_IQ_2) = 0;//FLOAT_TO_D16QN(-0.1, SPI_QN_IQ);
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
