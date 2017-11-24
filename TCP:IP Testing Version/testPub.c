#include"rms.h"


int main(void){
	masterIp = "127.0.0.1";
	char * stuffToPub = (char *)malloc(10*sizeof(char));
	strcpy(stuffToPub, "Hi Jason");
	unsigned int size[1];
	size[0] = 9;

	publish("topic", size, (void *)&stuffToPub, "127.0.0.1");

	while (1){

	}

	return 0;
}
