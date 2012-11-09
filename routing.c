#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <linux/in.h>
//#include <linux/ip.h>
#include <string.h>
#include <memory.h>
#include "routing.h"

const unsigned char ipempty[4] = {0, 0, 0, 0};
const unsigned char bcast[4] = {255, 255, 255, 255};
const unsigned char bcast_mac[6] = {255,255,255,255,255,255};

struct route_item route_table[MAX_ROUTE_INFO];
int routes = 0;

struct arp_item arp_table[MAX_ARP_INFO];
int arps = 0;

struct device_item device[MAX_DEVICE_INFO];
int devices = 0;


char buffer[BUFFER_MAX];
struct ethhdr eth_head;
struct arphdr2 arp_head;
int sock_fd, arp_fd;
int n_read;


int checkarp(unsigned char ipaddr[]){
	int i, ans = arps;

	for (i = 0; i < arps; i++){
		if (cmpip(arp_table[i].ip_addr, ipaddr))
			ans = i;

		if (arp_table[i].ttl < 100)
			arp_table[i].ttl ++;
	}
	if (ans < arps)
		return ans;
	else
		return -1;
}

int arpin(unsigned char macaddr[], unsigned char ipaddr[]){
	int i, j = 0;
	for (i = 0; i < arps; i++)
		if (cmpip(arp_table[i].ip_addr, ipaddr))
			return ;
	if (arps == MAX_ARP_INFO){
		for (i = 1; i < arps; i++)
			if (arp_table[i].ttl > arp_table[j].ttl)
				j = i;
	}
	else {
		j = arps;
		arps ++;
	}
	memcpy(arp_table[j].ip_addr, ipaddr, 4);
	memcpy(arp_table[j].mac_addr, macaddr, 6);
	arp_table[j].ttl = 0;

	return j;

}

int lookup(unsigned char ipaddr[], unsigned char p[]){
	int i;
	for (i = 0; i < routes; i++){
		if (subnet(route_table[i].destination, ipaddr, 
			route_table[i].netmask))
				break;
	}

	printf("i: %d routes: %d\n", i, routes);
	if (i == routes){
		printf("Don't know where to send when lookup.\n");
		return -1;
	}

/*
# check arp
# 1) check gateway's arp , need be route to somewhere else
# 2) or check it's arp, in the same subnet
*/

	if (checkip(route_table[i].gateway)) // case 2)
		memcpy(p, ipaddr, 4);
	else
		memcpy(p, route_table[i].gateway, 4);

	return i;

}

void router(char* buffer, int len, unsigned char src[], int iroute, int iarp){
/* encapsulate new packet */
	int i;
	struct sockaddr_ll dif;

	memset(&dif, 0, sizeof dif);

	for (i = 0; i < devices; i++)
		if (strcmp(device[i].interface, route_table[iroute].interface) == 0)
			break;
	
	memcpy(eth_head.h_source, device[i].mac_addr, ETH_ALEN);

	if (iarp == -1){
		memcpy(eth_head.h_dest, bcast_mac, ETH_ALEN);
		memcpy(arp_head.ar_sha, device[i].mac_addr, ETH_ALEN);
		memcpy(arp_head.ar_sip, device[i].ip_addr, INLEN);
		memcpy(arp_head.ar_tip, src, INLEN);
		eth_head.h_proto = htons(ETH_P_ARP);
		bufin(buffer, len);
	}
	else
		memcpy(eth_head.h_dest, arp_table[iarp].mac_addr, ETH_ALEN);

#ifdef DEBUG
	printf("new packet:\n");
	displaymac(eth_head.h_source);
	displaymac(eth_head.h_dest);
	printf("end new packet:\n");
#endif

	memcpy(buffer, &eth_head, sizeof eth_head);
	dif.sll_family = PF_PACKET;
	dif.sll_ifindex = if_nametoindex(device[i].interface);
	printf("%s\n", device[i].interface);
	
	if (iarp == -1){
		memcpy(&buffer[sizeof eth_head], &arp_head, sizeof arp_head);
		len = sizeof(eth_head) + sizeof(arp_head);
	}
		
	i = sendto(sock_fd, buffer, n_read, 0, (struct sockaddr*)&dif, sizeof dif);
	if (i <= 0){
		printf("Error while sending\n");
		exit(0);
	}
}

void ip_handle(char* p){
	struct iphdr ip_head;
	unsigned char addr[4];
	int iarp, iroute, i;

	memcpy(&ip_head, p, sizeof ip_head);
#ifdef DEBUG
	printf(" IP SRCADD: "); displayip(ip_head.saddr);
	printf(" IP DSTADD: "); displayip(ip_head.daddr);
#endif

	if (checkip(ip_head.daddr)){
		printf("No need to route\n");
		return ;
	}

	if (!passby(ip_head.daddr)){
		printf("Arrived\n");
		return ;
	}

	iroute = lookup(ip_head.daddr, addr);
	if (iroute == -1)
		return ;

	iarp = checkarp(addr);
	printf("iarp iroute: %d %d\n", iarp, iroute);

	router(buffer, n_read, addr, iroute, iarp);

#ifdef DEBUG
	printf("in ip_handle:\n");
	displayip(ip_head.daddr);
	displayip(addr);
#endif

}

void arp_handle(char* p){
	struct arphdr2 *arp_head = (struct arphdr2*)p;
	int i, j, iarp, iroute, len;
	char* buf; 
	unsigned char addr[4];

#ifdef DEBUG
	printf("arp packet: \n");
	displayip(arp_head->ar_sip);
	displaymac(arp_head->ar_sha);
#endif
	if (arp_head->ar_op != htons(ARPOP_REPLY))
		return ;

	iarp = arpin(arp_head->ar_sha, arp_head->ar_sip);
	iroute = lookup(arp_head->ar_sip, addr);
	printf(" iarp, iroute, : %d %d \n", iarp, iroute);

	for (i = 0; i < POOL_MAX; i++){
		if (checkbuf(arp_head->ar_sip, i, &buf, &len)){
			router(buf, len, addr, iarp, iroute);
			bufout(i);
		}
	}

}

void init_device_info(){
	char buf[100];
	int j, i; 

/* init devices info */
	FILE *fd = fopen("devices", "r");
	if (fd == NULL){
		printf("file devices does not exist\n");
		exit(1);
	}

	devices = 0; j = 0;
	while (fgets(device[j].interface, 99, fd) != NULL){
		nameclean(device[j].interface);
		fgets(buf, 99, fd);
		strmac(buf, device[j].mac_addr);
		fgets(buf, 99, fd);
		strip(buf, device[j].ip_addr);
		fgets(buf, 99, fd);
		strip(buf, device[j].mask);
		j ++;
	}

	devices = j;
#ifdef DEBUG
/* check */
	for (i = 0; i <devices; i++){	
		printf("%s\n", device[i].interface);
		displaymac(device[i].mac_addr);
		displayip(device[i].ip_addr);
		displayip(device[i].mask);
	}
#endif
	fclose(fd);
}

void init_route_info(){
/*test*/
	printf("init route_info\n");
	routes = 3;

	strip("192.168.3.0", route_table[0].destination);
	strip("255.255.255.0", route_table[0].netmask);
	memset(route_table[0].gateway, 0, 4);
	memcpy(route_table[0].interface, "eth1", 5);

	strip("192.168.2.0", route_table[1].destination);
	strip("255.255.255.0", route_table[1].netmask);
	memset(route_table[1].gateway, 0, 4);
	memcpy(route_table[1].interface, "eth2", 5);

	strip("192.168.186.0", route_table[2].destination);
	strip("255.255.255.0", route_table[2].netmask);
	memset(route_table[2].gateway, 0, 4);
	memcpy(route_table[2].interface, "eth0", 5);

#ifdef DEBUG
	printf("%s ", route_table[0].interface);
	printf("%s ", route_table[1].interface);
	printf("%s ", route_table[2].interface);
#endif
/*endtest*/
}

void init_arp_info(){
// for test
	arps = 2;
	strip("192.168.2.167", arp_table[0].ip_addr);
	strmac("00:0c:29:fc:27:fe", arp_table[0].mac_addr);
	strip("192.168.186.1", arp_table[1].ip_addr);
	strmac("00:50:56:c0:00:01", arp_table[1].mac_addr);
	strip("192.168.3.2", arp_table[2].ip_addr);
	strmac("00:0c:29:62:47:ac", arp_table[2].mac_addr);

	arp_head.ar_hrd = htons(0x0001);
	arp_head.ar_pro = htons(ETH_P_IP);
	arp_head.ar_hln = ETH_ALEN;
	arp_head.ar_pln = INLEN;
	arp_head.ar_op = htons(ARPOP_REQUEST);

}

int main(int argc, char* argv[]){
	unsigned char *p;
	int time = 10;

	init_device_info();
	init_route_info();
	init_arp_info();
	init_packbuf();

	if ((sock_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		printf("error create raw socket\n");
		return -1;
	}
	if ((arp_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP))) < 0){
		printf("error create raw socket\n");
		return -1;
	}

	while (time--) {
		/* create socket */
		n_read = recv(sock_fd, buffer, 2048, 0);
		if (n_read < 42){
			printf("error when recv msg\n");
			return -1;
		}
		printf(" received %d data\n", n_read);
		
		/* extract the macframe header */
		p = buffer;
		memcpy(&eth_head, p, sizeof eth_head);

		if (checkwho(eth_head.h_source))
			continue;

#ifdef DEBUG
		printf("Mac Address: \n");

		printf(" SRCMAC: "); 
			displaymac(eth_head.h_source);
		printf(" DSTMAC: "); 
			displaymac(eth_head.h_dest);
#endif

		p += sizeof eth_head;
		printf("%x\n", *p);

		switch (htons(eth_head.h_proto)){
			case ETH_P_IP: 
			//	printf("ip proto\n");
				ip_handle(p);
				break;

			case ETH_P_ARP:
			//	printf("arp proto\n");
				arp_handle(p);
				break;

			default:
				break;
		}

		printf("########NEWBLOCK########\n\n");
	}

	printf("arp_table FINAL SHOW !!!\n");
	int i;
	for (i = 0; i < arps; i++){
		displayip(arp_table[i].ip_addr);
		displaymac(arp_table[i].mac_addr);
	}
	return 0;
}
