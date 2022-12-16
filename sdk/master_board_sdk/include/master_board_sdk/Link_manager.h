#ifndef LINK_MANAGER_H
#define LINK_MANAGER_H

#include <string>
#include <thread>

#include "master_board_sdk/Link_types.h"

#define LEN_RAWBYTES_MAX 512

class LINK_manager_callback
{
	public:
	virtual void callback(uint8_t src_mac[6], uint8_t *data, int len) = 0;
};


struct thread_args
{
	int sock_fd = -1;
	LINK_manager_callback * obj_link_manager_callback;
	Packet_t *mypacket;
};


class LINK_manager
{
public:
	LINK_manager(Packet_t *packet_p) : mypacket(packet_p) {}
	LINK_manager(Packet_t *packet_p, const std::string &interface) : LINK_manager(packet_p)
	{
		set_interface(interface);
	}
	virtual ~LINK_manager();

	void set_recv_callback(LINK_manager_callback *);
	void set_interface(const std::string &interface);

	void start();
	virtual void stop();
	void end();

	int send(uint8_t *payload, int len);
	int send();

	void set_src_mac(uint8_t src_mac[6]);
	void set_dst_mac(uint8_t dst_mac[6]);

	Packet_t *mypacket;

protected:
	int sock_fd = -1;
	int socket_priority = 7;
	std::string interface;

	pthread_t recv_thd_id;
	struct thread_args recv_thread_params;
	static void *sock_recv_thread(void *p_arg);
};

#endif
