
#define MAX_ROUTE_INFO 20
#define MAX_ARP_INFO 20
#define MAX_DEVICE_INFO 10

#define BUFFER_MAX 1024 
#define POOL_MAX 5

#define ETH_ALEN 6
#define INLEN 4

#define IFLEN 14

#define bool unsigned char

#define true 1
#define false 0

#define DEBUG

/* the default gateway should always be the last */
struct route_item{
	unsigned char destination[4];
	unsigned char gateway[4];
	unsigned char netmask[4];
	unsigned char interface[14];
};

struct arp_item{
	unsigned char ip_addr[4];
	unsigned char mac_addr[6];
	int ttl;
};

struct device_item{
	unsigned char interface[14];
	unsigned char mac_addr[6];
	unsigned char ip_addr[4];
	unsigned char mask[4];
};

struct iphdr {
	unsigned char ihl:4,
		version:4;

	unsigned char tos;
	short tot_len;
	short id;
	short frag_off;
	unsigned char ttl;
	unsigned char protocol;
	short check;
	unsigned char saddr[4];
	unsigned char daddr[4];
};

struct arphdr2 {
	short ar_hrd;		/* format of hardware address	*/
	short ar_pro;		/* format of protocol address	*/
	unsigned char	ar_hln;		/* length of hardware address	*/
	unsigned char	ar_pln;		/* length of protocol address	*/
	short ar_op;		/* ARP opcode (command)		*/

	unsigned char		ar_sha[6];	/* sender hardware address	*/
	unsigned char		ar_sip[4];		/* sender IP address		*/
	unsigned char		ar_tha[6];	/* target hardware address	*/
	unsigned char		ar_tip[4];		/* target IP address		*/

};

extern int devices;
extern int routes;
extern int arps;

