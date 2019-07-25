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

spi_packet slaves_packets[6];

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

	
	memset(slaves_packets, 0, sizeof(spi_packet)*6);

	handler->set_dst_mac(dest_mac);
	
	int n_count = 0;
	std::chrono::time_point<std::chrono::system_clock> last;

	while(1) {
		if(((std::chrono::duration<double>) (std::chrono::system_clock::now() - last)).count() > 0.001) {
			if(n_count < 10) {
				slaves_packets[1].payload.control.cmd = 0x1;
		        slaves_packets[1].payload.control.payload.commands.present_flags.flags = 0;
		        slaves_packets[1].payload.control.payload.commands.present_flags.recv_Iq_timeout = 0;
		        slaves_packets[1].payload.control.payload.commands.flags_values.flags = 0;

		        slaves_packets[1].payload.control.payload.commands.present_flags.flags |= SPI_CONTROL_CMD_FLAGS_ENA_SYS | SPI_CONTROL_CMD_FLAGS_ENA_MTR1 | SPI_CONTROL_CMD_FLAGS_ENA_MTR2;
		        slaves_packets[1].payload.control.payload.commands.flags_values.flags |= SPI_CONTROL_CMD_FLAGS_ENA_SYS | SPI_CONTROL_CMD_FLAGS_ENA_MTR1 | SPI_CONTROL_CMD_FLAGS_ENA_MTR2;
			} else {
				slaves_packets[1].payload.control.cmd = 0x0;
		        slaves_packets[1].payload.control.payload.commands.present_flags.flags = 0;
		        slaves_packets[1].payload.control.payload.commands.present_flags.recv_Iq_timeout = 0;
		        slaves_packets[1].payload.control.payload.commands.flags_values.flags = 0;

				slaves_packets[1].payload.control.payload.values.Iq[0] = 25;
				slaves_packets[1].payload.control.payload.values.Iq[1] = -25;
			}
			
			last = std::chrono::system_clock::now();

			handler->send((uint8_t *) slaves_packets, sizeof(spi_packet)*6),

			n_count++;
		} else {
			std::this_thread::yield();
		}
	}

	handler->end();
}
