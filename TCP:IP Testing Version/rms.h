#include<stdlib.h>
#include<stdio.h>
#include<nanomsg/nn.h>
#include<nanomsg/tcp.h>
#include<nanomsg/reqrep.h>
#include<nanomsg/pair.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<nanomsg/pubsub.h>
#include<nanomsg/inproc.h>

/*
Ports:
8000: Subscribers
8001: Publishers
8002: Service
8003: ServiceReq
8004: Action
8005: ActionReq
*/

char *ip;
char *masterIp;

typedef struct{
	char * name;
	void **info;
	int maxSize;
	short int timed;
} SubInfo;

typedef struct{
	void **info;
	int maxSize;
	char * file;
	unsigned int timed;
}SubListenInfo;

void * subListenThread(void *stuff){
	//connect to published
	SubListenInfo * tmp = (SubListenInfo*)stuff;
	char newFile[100];
	strcpy(newFile, tmp[0].file);
	void ** info = tmp[0].info;
	int maxSize = tmp[0].maxSize;
	unsigned int timed = tmp[0].timed;
	int S1 = nn_socket(AF_SP, NN_SUB);
	if ((nn_connect(S1, newFile)) < 0){
		fprintf(stderr, "Error: failed to connect to publisher\n%s\n", strerror(errno));
		exit(1);
	}
	free(stuff);
	nn_setsockopt(S1, NN_SUB, NN_SUB_SUBSCRIBE, NULL, 0);
	while(1){
		//sleep if requested
		sleep(timed);
		if(*info == NULL){
			*info = malloc(maxSize);
			nn_recv(S1, *info, maxSize, 0);
		} else {
			nn_recv(S1, *info, maxSize, 0);
		}
	}
	nn_close(S1);
	return NULL;
}

void * subThread(void *stuff){
	SubInfo * tmpStuff = (SubInfo*)stuff;
	char *name = tmpStuff[0].name;
	void ** info = tmpStuff[0].info;
	int maxSize = tmpStuff[0].maxSize;
	unsigned int timed = tmpStuff[0].timed;

	char masterSub[30] = "tcp://";
	strcat(masterSub, masterIp);
	strcat(masterSub, ":8000");
	//connect to master
	int S1 = nn_socket(AF_SP, NN_REQ);
	if ((nn_connect(S1, masterSub)) < 0){
		fprintf(stderr, "Error: failed to bind to master sub port\n%s\n", strerror(errno));
		exit(1);
	}
	sleep(1);
	//send name to master
	if ((nn_send(S1, name, strlen(name), 0)) < 0){
		fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
		exit(1);
	}

	if ((nn_send(S1, ip, strlen(ip), 0)) < 0){
		fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
		exit(1);
	}
	//recieve new address
	char recvBuff[20];
	if((nn_recv(S1, recvBuff, 20, 0)) < 0){
		fprintf(stderr, "Error: failed to be get new listening ip\n%s\n", strerror(errno));
		exit(1);
	}
	nn_close(S1);
	//-------------hop to pair with master-----------------
	char newAddr[23];
	strcat(newAddr, "tcp://");
	strcat(newAddr, recvBuff);
	
	int S2 = nn_socket(AF_SP, NN_PAIR);
	if((nn_connect(S2, newAddr)) < 0){
		fprintf(stderr, "Error: failed to connect pair\n%s\n",strerror(errno));
		exit(1);
	}
	while(1){
		//get the address of all publisher to the topic
		char fileToSubTo[100];
		nn_recv(S2, fileToSubTo, 20, 0);
		char addrToSub[23] = "tcp://";
		strcat(addrToSub, fileToSubTo);
		pthread_t SLT;
		SubListenInfo * tmp = (SubListenInfo *)malloc(sizeof(SubListenInfo));
		tmp[0].file = addrToSub;
		tmp[0].info = info;
		tmp[0].maxSize = maxSize;
		tmp[0].timed = timed;
		//spin up new thread to subscribe
		pthread_create(&SLT, NULL, subListenThread, (void *)tmp);
		sleep(1);
	}
	nn_close(S2);
	return NULL;
}

void subscribe(char * subName, void **info, int maxSize, char * myIp){
	//spin up new listening thread
	pthread_t subNode;
	SubInfo * tmp = (SubInfo *)malloc(sizeof(SubInfo));
	tmp[0].name = subName;
	tmp[0].info = info;
	tmp[0].maxSize = maxSize;
	tmp[0].timed = 0;
	ip = myIp;
	pthread_create(&subNode, NULL, subThread, (void *)tmp);
	sleep(1);
}

void timedSubscribe(char * subName, void **info, int maxSize, int time, char * myIp){
	//spin up new listening thread
    pthread_t subNode;
    SubInfo * tmp = (SubInfo *)malloc(sizeof(SubInfo));
    tmp[0].name = subName;
    tmp[0].info = info;
    tmp[0].maxSize = maxSize;
	tmp[0].timed = time;
	ip = myIp;
    pthread_create(&subNode, NULL, subThread, (void *)tmp);
    sleep(1);
}

//returns return of the remote function
void * serviceRequest(char * name, void * stuff, int size, char *myIp){
	ip = myIp;
	//connect to master
	int S1 = nn_socket(AF_SP, NN_REQ);
	if((nn_connect(S1, "tcp://127.0.0.1:8003")) < 0){
		fprintf(stderr, "Error: failed to connect to master service request\n%s\n", strerror(errno));
		exit(1);
	}
	if((nn_send(S1, name, strlen(name)+1, 0)) < 0){
		fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
		exit(1);
	}
	char file[23] = "tcp://127.0.0.1:";
	char recvBuff[9];
	if((nn_recv(S1, recvBuff, 9, 0)) < 0){
		fprintf(stderr, "Error: failed to receive new file from master\n%s\n", strerror(errno));
		exit(1);
	}
	strcat(file, recvBuff);
	nn_close(S1);
	//connect to pair with master to get address of service
	int S2 = nn_socket(AF_SP, NN_PAIR);
	if((nn_connect(S2, file)) < 0){
		fprintf(stderr, "Error: failed to connect to new master pair\n%s\n", strerror(errno));
		exit(1);
	}
	char newFile[23] = "tcp://127.0.0.1:";
	if((nn_recv(S2, recvBuff, 9, 0)) < 0){
		fprintf(stderr, "Error: failed to recieve service port\n%s\n", strerror(errno));
		exit(1);
	}
	strcat(newFile, recvBuff);
	nn_close(S2);
	//connect to service
	int S3 = nn_socket(AF_SP, NN_REQ);
	if((nn_connect(S3, newFile)) < 0){
		fprintf(stderr, "Error: failed to connect to service\n%s\n", strerror(errno));
		exit(1);
	}
	//send request
	if((nn_send(S3, stuff, size, 0)) < 0){
		fprintf(stderr, "Error: failed to send perameters to action server\n%s\n", strerror(errno));
		exit(1);
	}
	//recieve return of the service
	void * tmp;
	if ((nn_recv(S3, &tmp, NN_MSG, 0)) < 0){
		fprintf(stderr, "Error: failed to recieve return value\n%s\n", strerror(errno));
		exit(1);
	}
	nn_close(S3);
	return tmp;

}

void actionRequest(char * name, void * stuff, int size, void ** returnMsgBuff, char * myIp){
    //connect to master
	ip = myIp;
    int S1 = nn_socket(AF_SP, NN_REQ);
    if((nn_connect(S1, "tcp://127.0.0.1:8005")) < 0){
        fprintf(stderr, "Error: failed to connect to master\n%s\n", strerror(errno));
        exit(1);
    }
    if((nn_send(S1, name, strlen(name)+1, 0)) < 0){
        fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
        exit(1);
    }
    char file[23] = "tcp://127.0.0.1:";
    char recvBuff[9];
    if((nn_recv(S1, recvBuff, 9, 0)) < 0){
        fprintf(stderr, "Error: failed to receive new port from master\n%s\n", strerror(errno));
        exit(1);
    }
    strcat(file, recvBuff);
    nn_close(S1);
    //connect to pair with master to get address of action server
    int S2 = nn_socket(AF_SP, NN_PAIR);
    if((nn_connect(S2, file)) < 0){
        fprintf(stderr, "Error: failed to connect to new master pair\n%s\n", strerror(errno));
        exit(1);
    }
    char newFile[23] = "tcp://127.0.0.1:";
    if((nn_recv(S2, recvBuff, 9, 0)) < 0){
        fprintf(stderr, "Error: failed to recieve action server port\n%s\n", strerror(errno));
	}
    strcat(newFile, recvBuff);
    nn_close(S2);
    //connect to action server
    int S3 = nn_socket(AF_SP, NN_PAIR);
    if((nn_connect(S3, newFile)) < 0){
        fprintf(stderr, "Error: failed to connect to action server\n%s\n", strerror(errno));
        exit(1);
    }
    //send perrameters
    if((nn_send(S3, stuff, size, 0)) < 0){
        fprintf(stderr, "Error: failed to send perameters to action server\n%s\n", strerror(errno));
        exit(1);
    }
	//recv intermediate comm
	while(1){
		void * recvBuff2;
		int N1 = nn_recv(S3, &recvBuff2, NN_MSG, 0);
		if(N1 < 0){
			fprintf(stderr, "Error: failed to recieve from action server\n%s\n", strerror(errno));
			exit(1);
		}
		if(N1 == 19){
			if(strncmp(recvBuff2, "end of transmition", 19) == 0){
				nn_freemsg(recvBuff2);
				break;
			}
		}
		if(*returnMsgBuff == NULL){
			*returnMsgBuff = malloc(N1);
			memcpy(*returnMsgBuff, recvBuff2, N1);
		} else {
			free(*returnMsgBuff);
			*returnMsgBuff = malloc(N1);
			memcpy(*returnMsgBuff, recvBuff2, N1);
		}
		nn_freemsg(recvBuff2);
	}
    nn_close(S3);
    return;

}

short int inprocFlag = 0;
void * inprocData = NULL;
short int inprocChangeFlag = 0;
unsigned int inprocSize = 0;

typedef struct{
	char * name;
	unsigned int * n;
	void ** pubBuff;
	unsigned int timedPub;
} PubInfo;

void * pubThread(void * stuff){
	//connect to master node
	PubInfo * tmpStuff = (PubInfo *)stuff;
	char masterPub[30] = "tcp://";
	strcat(masterPub, masterIp);
	strcat(masterPub, ":8001");
	int S1 = nn_socket(AF_SP, NN_REQ);
	if ((nn_connect(S1, masterPub)) < 0){
		fprintf(stderr, "Error: failed to bind to master\n%s\n", strerror(errno));
		exit(1);
	}
	sleep(1);
	//sond name to master
	if ((nn_send(S1, tmpStuff[0].name, strlen(tmpStuff[0].name), 0)) < 0){
		fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
		exit(1);
	}
	if ((nn_send(S1, ip, strlen(ip), 0)) < 0){
		fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
		exit(1);
	}
	char recvBuff[20];
	//recieve new ip to publish to
	if ((nn_recv(S1, recvBuff, 20, 0)) < 0){
		fprintf(stderr, "Error: failed to recieve port for publishing\n%s\n", strerror(errno));
		exit(1);
	}
	nn_close(S1);
	//printf("recv: %s", recvBuff);
	//connect to new file
	char newAddr[23];
	strcat(newAddr, "tcp://");
	strcat(newAddr, recvBuff);
	int S2 = nn_socket(AF_SP, NN_PUB);
	if(S2 < 0){
		fprintf(stderr, "Error: failed to open socket\n%s\n", strerror(errno));
		exit(1);
	}
	if ((nn_bind(S2, newAddr)) < 0){
		fprintf(stderr, "Error: failed to bind to new address\n%s\n", strerror(errno));
		exit(1);
	}
	//begin publishing
	while (1){
		nn_send(S2, *(tmpStuff[0].pubBuff), *(tmpStuff[0].n), 1);
	}
	nn_close(S2);
	return NULL;
}




void * pubTimedThread(void * stuff){
	//connect to master
    PubInfo * tmpStuff = (PubInfo *)stuff;
    int S1 = nn_socket(AF_SP, NN_REQ);
    if ((nn_connect(S1, "tcp://127.0.0.1:8001")) < 0){
        fprintf(stderr, "Error: failed to bind to master\n%s\n", strerror(errno));
        exit(1);
    }
    sleep(1);
	//send name to master
    if ((nn_send(S1, tmpStuff[0].name, strlen(tmpStuff[0].name), 0)) < 0){
        fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
        exit(1);
    }
	//recieve file for publishing
    char recvBuff[9];
    if ((nn_recv(S1, recvBuff, 9, 0)) < 0){
        fprintf(stderr, "Error: failed to recieve new port for publishing\n%s\n", strerror(errno));
        exit(1);
    }
    nn_close(S1);
    char newAddr[23] = "tcp://127.0.0.1:";
    strcat(newAddr, recvBuff);
    //connect to file for publishing
	int S2 = nn_socket(AF_SP, NN_PUB);
    if(S2 < 0){
        fprintf(stderr, "Error: failed to bind to publishing port\n%s\n", strerror(errno));
        exit(1);
    }
    if ((nn_bind(S2, newAddr)) < 0){
        fprintf(stderr, "Error: failed to bind to new address\n%s\n", strerror(errno));
        exit(1);
    }
	//publish and sleep
    while (1){
		usleep(tmpStuff[0].timedPub);
		nn_send(S2, *(tmpStuff[0].pubBuff), *(tmpStuff[0].n), 0);
    }
    nn_close(S2);
    return NULL;
}

void * nonInteruptThread(void * stuff){
	//connect to master
	PubInfo * tmpStuff = (PubInfo *)stuff;
    int S1 = nn_socket(AF_SP, NN_REQ);
	if ((nn_connect(S1, "tcp://127.0.0.1:8001")) < 0){
        fprintf(stderr, "Error: failed to bind to master\n%s\n", strerror(errno));
        exit(1);
    }
    sleep(1);
	//send name to master
    if ((nn_send(S1, tmpStuff[0].name, strlen(tmpStuff[0].name), 0)) < 0){
        fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
        exit(1);
    }
	//recieve file to publish to
    char recvBuff[9];
    if ((nn_recv(S1, recvBuff, 9, 0)) < 0){
        fprintf(stderr, "Error: failed to recieve port for publishing\n%s\n", strerror(errno));
        exit(1);
    }
    nn_close(S1);
    char newAddr[23] = "tcp://127.0.0.1:";
    strcat(newAddr, recvBuff);
    //connect to new file for publishing
    int S2 = nn_socket(AF_SP, NN_PUB);
    if(S2 < 0){
        fprintf(stderr, "Error: failed to bind to publishing port\n%s\n", strerror(errno));
        exit(1);
    }
    if ((nn_bind(S2, newAddr)) < 0){
        fprintf(stderr, "Error: failed to bind to new address\n%s\n", strerror(errno));
        exit(1);
    }
	//publish in blocking mode
    while (1){
        nn_send(S2, *(tmpStuff[0].pubBuff), *(tmpStuff[0].n), 0);
    }
    nn_close(S2);
    return NULL;
}


//n is size of data in bytes
/*
takes the name of a topic to publish to, the number of bytes, and the thing to be published. 
*/
void publish(char * name, unsigned int *n, void **thingToPub, char *myIp){
	pthread_t pubNode;
	PubInfo * tmp = (PubInfo *)malloc(sizeof(PubInfo));
	tmp[0].name = name;
	//needs to be a pointer so that size can change
	tmp[0].n = n;
	tmp[0].pubBuff = thingToPub;
	ip = myIp;
	pthread_create(&pubNode, NULL, pubThread, (void *)tmp);
}

//time is in microseconds
void timedPublish(char * name, unsigned int *n, void **thingToPub, int timeBetweenSend, char * myIp){
    pthread_t pubNode;
    PubInfo * tmp = (PubInfo *)malloc(sizeof(PubInfo));
    tmp[0].name = name;
    //needs to be a pointer so that size can change
    tmp[0].n = n;
    tmp[0].pubBuff = thingToPub;
	tmp[0].timedPub = timeBetweenSend;
	ip = myIp;
    pthread_create(&pubNode, NULL, pubTimedThread, (void *)tmp);
}

void nonInteruptedPublish(char * name, unsigned int *n, void **thingToPub, char * myIp){
    pthread_t pubNode;
    PubInfo * tmp = (PubInfo *)malloc(sizeof(PubInfo));
    tmp[0].name = name;
    tmp[0].n = n;
    tmp[0].pubBuff = thingToPub;
	ip = myIp;
    pthread_create(&pubNode, NULL, nonInteruptThread, (void *)tmp);
}

//takes a topic, a pointer to a function to call when server gets a request, and takes the size in bytes of the return function
void service(char * name, void *(*fp)(void *), unsigned int sizeOfReturn, char * myIp){
	//connect to master
	ip = myIp;
	int S1 = nn_socket(AF_SP, NN_REQ);
	if((nn_connect(S1, "tcp://127.0.0.1:8002")) < 0){
		fprintf(stderr, "Error: failed to connect to master\n%s\n", strerror(errno));
		exit(1);
	}
	//send name to master
	if ((nn_send(S1, name, strlen(name), 0)) < 0){
		fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
		exit(1);
	}
	//recieve new file 
	char recvBuff[9];
	if((nn_recv(S1, recvBuff, 9, 0)) < 0){
		fprintf(stderr, "Error: failed to receive new port\n%s\n", strerror(errno));
		exit(1);
	}
	//connect to new file for requests
	nn_close(S1);
	int S2 = nn_socket(AF_SP, NN_REP);
	char file[23] = "tcp://127.0.0.1:";
	strcat(file, recvBuff);
	if((nn_bind(S2, file)) < 0){
		fprintf(stderr, "Error: failed to open new port for listening\n%s\n", strerror(errno));
		exit(1);
	}
	//listen for requests and call function
	while(1){
		void * perram = NULL;
		if((nn_recv(S2, &perram, NN_MSG, 0)) < 0){
			fprintf(stderr, "Error: failed to receive message\n%s\n", strerror(errno));
			exit(1);
		}
		void * ret = fp(perram);
		nn_freemsg(perram);
		//send return of funciton to caller
		if((nn_send(S2, ret, sizeOfReturn, 0)) < 0){
			fprintf(stderr, "Error: failed to send return of function\n%s\n", strerror(errno));
			exit(1);
		}
	}
}


void actionServer(char * name, void *(*fp)(void *), unsigned int sizeOfReturn, char *myIp){
	//connect to master
	ip=myIp;
    int S1 = nn_socket(AF_SP, NN_REQ);
    if((nn_connect(S1, "tcp://127.0.0.1:8004")) < 0){
        fprintf(stderr, "Error: failed to connect to master\n%s\n", strerror(errno));
        exit(1);
    }
    //send name to master
    if ((nn_send(S1, name, strlen(name), 0)) < 0){
        fprintf(stderr, "Error: failed to send name to master\n%s\n", strerror(errno));
        exit(1);
    }
    //recieve new file 
    char recvBuff[9];
    if((nn_recv(S1, recvBuff, 9, 0)) < 0){
        fprintf(stderr, "Error: failed to receive new port\n%s\n", strerror(errno));
        exit(1);
    }
    //connect to new file for requests
    nn_close(S1);
    int S2 = nn_socket(AF_SP, NN_PAIR);
    char file[23] = "tcp://127.0.0.1:";
    strcat(file, recvBuff);
    if((nn_bind(S2, file)) < 0){
        fprintf(stderr, "Error: failed to open new port for listening\n%s\n", strerror(errno));
        exit(1);
    }
    //listen for requests and call function
    while(1){
        void * tmp = NULL;
        if((nn_recv(S2, &tmp, NN_MSG, 0)) < 0){
            fprintf(stderr, "Error: failed to receive message\n%s\n", strerror(errno));
            exit(1);
        }
        //create thread for function
		pthread_t TAid;
		pthread_create(&TAid, NULL, fp, tmp);
		inprocFlag = 0;
        //listen for action send and return 
		while (1){
			if(inprocFlag){
				goto breakToOuterLoop;
			}
			if(inprocChangeFlag){
                //needs to wait otherwise nanomsg cannot send/recv properly
				usleep(10);
                //send data to caller
				if ((nn_send(S2, inprocData, inprocSize, 0)) < 0){
					fprintf(stderr, "Error: failed to send data\n%s\n", strerror(errno));
					exit(1);
				} else {
					free(inprocData);
					inprocData = NULL;
					inprocSize = 0;
					inprocChangeFlag = 0;
				}
			}
		}
        //end connection
		breakToOuterLoop:
		inprocFlag = 1;
		void * funcReturn;
		pthread_join(TAid, funcReturn);
		nn_send(S2, "end of transmition", 19, 0 );
		nn_send(S2, funcReturn, sizeOfReturn, 0);
	}
	nn_close(S2);	
}

void actionSend(void * stuff, unsigned int size){
	//wait until the last data has been processed to add the data to the list
	do{
		if(!inprocChangeFlag){
			inprocChangeFlag = 1;
			inprocData = malloc(size);
			memcpy(inprocData, stuff, size);
			inprocSize = size;
			return;
		}
	}while(1);
}


void actionReturn(void *stuff, unsigned int size){
	//wait until the last data has been processed to add the data to the list, then break out of the listening loop
	do{
		if(!inprocChangeFlag){
		    inprocChangeFlag = 1;
    		inprocData = malloc(size);
			memcpy(inprocData, stuff, size);
		    inprocSize = size;
			inprocFlag = 1;
			return;
		}
	}while(1);
}
typedef struct{
	char * nameOfTopic;
	char * nameOfPub;
}InitStuff;
