#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_arp.h>
#include <arpa/inet.h>
#include <assert.h>

#include "master_board_sdk/Link_manager.h"

void LINK_manager::set_interface(const std::string &interface)
{
	this->interface = interface;
}

void LINK_manager::set_src_mac(uint8_t src_mac[6])
{
	mypacket->set_src_mac(src_mac);
}

void LINK_manager::set_dst_mac(uint8_t dst_mac[6])
{
	mypacket->set_dst_mac(dst_mac);
}

void LINK_manager::set_recv_callback(LINK_manager_callback * obj_link_manager_callback)
{
	recv_thread_params.obj_link_manager_callback = obj_link_manager_callback;
}

void LINK_manager::start()
{
	struct sockaddr_ll s_dest_addr;
	struct ifreq ifr;

	int fd,							//file descriptor
			ioctl_errno,		//ioctl errno
			bind_errno,			//bind errno
			priority_errno; //Set priority errno

	bzero(&s_dest_addr, sizeof(s_dest_addr));
	bzero(&ifr, sizeof(ifr));
	fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	assert(fd != -1);

	strncpy((char *)ifr.ifr_name, this->interface.c_str(), IFNAMSIZ); //interface

	ioctl_errno = ioctl(fd, SIOCGIFINDEX, &ifr);
	assert(ioctl_errno >= 0); //abort if error

	s_dest_addr.sll_family = PF_PACKET;
	s_dest_addr.sll_protocol = htons(ETH_P_ALL);
	s_dest_addr.sll_ifindex = ifr.ifr_ifindex;

	bind_errno = bind(fd, (struct sockaddr *)&s_dest_addr, sizeof(s_dest_addr));
	assert(bind_errno >= 0); //abort if error

	priority_errno = setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &(this->socket_priority), sizeof(this->socket_priority));
	assert(priority_errno == 0);

	this->sock_fd = fd;

	this->recv_thread_params.sock_fd = this->sock_fd;
	this->recv_thread_params.mypacket = this->mypacket;

	pthread_create(&recv_thd_id, NULL, &(LINK_manager::sock_recv_thread), &recv_thread_params);
}

void LINK_manager::stop()
{
	if (recv_thd_id)
	{
		pthread_cancel(recv_thd_id);
		pthread_join(recv_thd_id, NULL);
		recv_thd_id = 0;
	}
	if (this->sock_fd > 0)
	{
		close(this->sock_fd);
		this->sock_fd = -1;
	}
}

void LINK_manager::end()
{
	stop();
}


LINK_manager::~LINK_manager()
{
	end();
}

void *LINK_manager::sock_recv_thread(void *p_arg)
{
	int raw_bytes_len;
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];

	uint8_t *res_mac;
	uint8_t *res_payload;
	int res_len;

	struct thread_args *params = (struct thread_args *)p_arg;

	if (params->obj_link_manager_callback == NULL)
	{
		printf("No callback for receive, receive thread exited\n");
		return EXIT_SUCCESS;
	};

	while (1)
	{
		raw_bytes_len = static_cast<int>(recvfrom(params->sock_fd, raw_bytes, LEN_RAWBYTES_MAX, MSG_TRUNC, NULL, 0));

		if (raw_bytes_len < 0)
		{
			perror("Socket receive, error ");
		}
		else
		{
			res_mac = params->mypacket->get_src_mac_FromRaw(raw_bytes, raw_bytes_len);
			res_payload = params->mypacket->get_payload_FromRaw(raw_bytes, raw_bytes_len);
			res_len = params->mypacket->get_payload_len_FromRaw(raw_bytes, raw_bytes_len);
			if (res_mac != NULL && res_payload != NULL && res_len > 0)
			{
				params->obj_link_manager_callback->callback(res_mac, res_payload, res_len);
			}
		}
	}

	printf("Receive thread exited \n");
	return EXIT_SUCCESS;
}

int LINK_manager::send(uint8_t *payload, int len)
{
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];

	//Not the most fastest way to do this :
	//	copy the payload in the packet array and then copy it back into the buffer...
	this->mypacket->set_payload_len(len);
	memcpy(this->mypacket->get_payload_ptr(), payload, len);

	int raw_len = mypacket->toBytes(raw_bytes, LEN_RAWBYTES_MAX);

	return static_cast<int>(sendto(this->sock_fd, raw_bytes, raw_len, 0, NULL, 0));
}

int LINK_manager::send()
{
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];

	int raw_len = mypacket->toBytes(raw_bytes, LEN_RAWBYTES_MAX);

	return static_cast<int>(sendto(this->sock_fd, raw_bytes, raw_len, 0, NULL, 0));
}
