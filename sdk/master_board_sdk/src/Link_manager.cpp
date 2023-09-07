#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#ifdef __linux__
#include <linux/if_arp.h>
#include <linux/if_packet.h>
#else
#include <net/if_arp.h>
#endif

#include <ifaddrs.h>
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
#ifdef __APPLE__
        int fd,							//file descriptor
            bind_errno,			//bind errno
            priority_errno; //Set priority errno

        /// Creates a raw socket
	fd = socket(PF_NDRV, SOCK_RAW, 0);
	assert(fd != -1);

        /// Find the interface specified in the argument interface.
        struct ifaddrs *ifap = NULL;
        bool found_if=false;

        if(getifaddrs(&ifap) < 0) {
          printf("Cannot get a list of interfaces\n");
          return;
        }

        for(struct ifaddrs *p = ifap; p!=NULL; p=p->ifa_next) {
          if (strcmp(p->ifa_name, interface.c_str()) == 0)
          {
            found_if=true;
            break; // Go out of the for loop
          }
        }

        if(!found_if)
        {
          printf("Did not find interface %s\n",interface.c_str());
          return;
        }
        freeifaddrs(ifap);

        /// Indicates that this program wants to use a raw socket (PF_NDRV)
        /// wants to use interface as interface.
	this->sa_ndrv.snd_family = PF_NDRV;
        this->sa_ndrv.snd_len = sizeof (sa_ndrv);
        strlcpy((char *)sa_ndrv.snd_name, interface.c_str(), sizeof (sa_ndrv.snd_name));

	bind_errno = bind(fd, (struct sockaddr *)&sa_ndrv, sizeof(sa_ndrv));
	if (bind_errno < 0)
        {
          printf("Unable to bind to %s\n",sa_ndrv.snd_name);
          perror("bind:");
          return;
        }

	priority_errno = setsockopt(fd, SOL_SOCKET, SO_NET_SERVICE_TYPE , &(this->socket_priority), sizeof(this->socket_priority));


        // Need to set this option for receiving Ethertype packet send from the
        // masterboard
        struct ndrv_protocol_desc desc;
        struct ndrv_demux_desc demux_desc[1];
        memset(&desc, '\0', sizeof(desc));
        memset(&demux_desc, '\0', sizeof(demux_desc));

        /* Request kernel for demuxing of one chosen ethertype */
        desc.version = NDRV_PROTOCOL_DESC_VERS;
        desc.protocol_family = 0x88b5; // The masterboard Ethertype.
        desc.demux_count = 1;
        desc.demux_list = (struct ndrv_demux_desc*)&demux_desc;
        demux_desc[0].type = NDRV_DEMUXTYPE_ETHERTYPE;
        demux_desc[0].length = sizeof(unsigned short);
        demux_desc[0].data.ether_type = ntohs(0x88b5);

        if (setsockopt(fd,
                       SOL_NDRVPROTO,
                       NDRV_SETDMXSPEC,
                       (caddr_t)&desc, sizeof(desc))) {
          perror("setsockopt"); exit(4);
         }
#else
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
#endif
        if (priority_errno < 0)
        {
          perror("Unable to start because the program could not set priority on low level link");
          assert(false);
        }

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
	memcpy(this->mypacket->get_payload_ptr(), payload,
               static_cast<size_t>(len));

	int raw_len = mypacket->toBytes(raw_bytes, LEN_RAWBYTES_MAX);

	return static_cast<int>(sendto(this->sock_fd, raw_bytes,
                                       static_cast<size_t>(raw_len), 0,
#ifdef __APPLE__
                                       (struct sockaddr *)&sa_ndrv,
                                       sizeof(sa_ndrv))
#else
                                NULL,0)
#endif
                                );
}

int LINK_manager::send()
{
	uint8_t raw_bytes[LEN_RAWBYTES_MAX];

	int raw_len = mypacket->toBytes(raw_bytes, LEN_RAWBYTES_MAX);

	return static_cast<int>(sendto(this->sock_fd, raw_bytes,
                                       static_cast<size_t>(raw_len), 0,
#ifdef __APPLE__
                                       (struct sockaddr *)&sa_ndrv, sizeof(sa_ndrv))
#else
                                NULL,0)
#endif
                                );
}
