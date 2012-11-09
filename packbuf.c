#include "routing.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/if_ether.h>



struct packpool{
	struct ethhdr eth_head;
	struct iphdr ip_head;
	char buf[990];
};

int packlen[POOL_MAX], live[POOL_MAX];
struct packpool* pack[POOL_MAX];
int cycle = 0, cnt = 0;


void init_packbuf(){
	memset(packlen,  0, sizeof packlen);
}

void bufin(char* p, int len){
	int i, j;
	for (i = 0; i < POOL_MAX; i++){
/*
		if (live[i] == cycle)
			j = i;
*/
		if (packlen[i] == 0)
			break;
	}

	if (i == POOL_MAX){
/*
		memcpy(pack[j], p, sizeof(struct packpool));
		packlen[i] = len;
		cycle ++;
*/
		return ;
	}
	else {
		pack[i] = (struct packpool*)malloc(sizeof(struct packpool));
		memcpy(pack[i], p, sizeof(struct packpool));
		if (pack[i] == NULL)
			return ;

		packlen[i] = len;
		printf("in bufin\n");
		displayip(pack[i]->ip_head.saddr);
		displayip(pack[i]->ip_head.daddr);
		/*
		live[i] = cnt;
		cnt = (cnt + 1) % POOL_MAX;
		*/
	}

}

void bufout(int index){
	free(pack[index]);
	packlen[index] = 0;
}

bool checkbuf(unsigned char addr[], int i, char** buf, int* len){
	if (packlen[i] == 0)
		return false;

	printf("checkbuf\n");
	displayip(addr);
	displayip(pack[i]->ip_head.daddr);
	displayip(pack[i]->ip_head.saddr);
	if (cmpip(pack[i]->ip_head.daddr, addr)){
		*len = packlen[i];
		*buf = (char*)(pack[i]);
		printf("true\n");
		return true;
	}

	return false;
}
