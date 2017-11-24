#include"rms.h"
#include<stdio.h>

int main(void){
	masterIp = "127.0.0.1";
	char * buff = (char *)malloc(50*sizeof(char));

	subscribe("topic", (void *)&buff, 50, "127.0.0.1");
	while(1){
		printf("Recv: %s\n", buff);
	}

	return 0;
}
