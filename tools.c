

#include <stdio.h>
#include "routing.h"

extern struct device_item device[];

int checkwho(unsigned char src[]){
	int i, j;
	int flag = false;
	
	for (i = 0; i < devices; i++){
		flag = true;
		for (j = 0; j < ETH_ALEN; j++){
			if (device[i].mac_addr[j] != src[j]){
				flag = false;
				break;
			}
		}
		if (flag)
			return true;
	}
	return false;
}

unsigned char checkip(unsigned char dst[]){
	int i;
	/* check if the packet is broadcast */
	if (dst[4] == 0 || dst[4] == 255)
		return true;
	return false;
}

void displaymac(unsigned char macaddr[]){
	int i;
	
	for (i = 0; i < ETH_ALEN; i++){
		if (i != ETH_ALEN-1)
			printf("%02x:", macaddr[i]);
		else
			printf("%02x\n", macaddr[i]);
	}
}

void displayip(unsigned char ipaddr[]){
	int i;
	for (i = 0; i < 4; i++){
		if (i != 3) 
			printf("%d.", ipaddr[i]);
		else
			printf("%d\n", ipaddr[i]);
	}
}

int hexchar(char x){
	int j = x;
	if (j >= '0' && j <= '9')
		j = j - '0';
	else 
		j = j - 'a' + 10;

	return j;
}

void strmac(char* buf, unsigned char addr[]){
	int i, j;
	for (i = 0; i < 6; i++){
		addr[i] = hexchar(buf[i*3])*16 + hexchar(buf[i*3+1]);
	}
}


void strip(char* buf, unsigned char addr[]){
	int i = 0, j;
//	printf("stripbuf: %s\n", buf);
	for (j = 0; j < 4; j++){
		while (isdigit(buf[i])){
			addr[j] = addr[j] * 10 + buf[i] - '0';
			i ++;
		}
		i++;
	}
}

int cmpip(unsigned char ip1[], unsigned char ip2[]){
	int i;
	for (i = 0; i < 4; i++){
		if (ip1[i] != ip2[i])
			return false;
	}
	return true;
}

int subnet(unsigned char ip1[], unsigned char ip2[], unsigned char mask[]){
	int i;
	for (i = 0; i < 4; i++){
		if ((ip1[i] & mask[i]) != (ip2[i] & mask[i]))
			return false;
		
	}
	return true;
}

bool passby(unsigned char addr[]){
	int i;
	for (i = 0; i < devices; i++){
		if (cmpip(addr, device[i].ip_addr))
			return false;
	}
	return true;
}

void nameclean(char *p){
	p[4] = 0;
}
