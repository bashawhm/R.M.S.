#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<signal.h>
#include<unistd.h>
#include<nanomsg/nn.h>
#include<nanomsg/ipc.h>
#include<nanomsg/reqrep.h>
#include<nanomsg/pair.h>

//how many of each type of node there are on the system
short unsigned int totalConnectNum = 0;
short unsigned int pubConnectNum = 0;
short unsigned int subConnectNum = 0;
short unsigned int serviceConnectNum = 0;
short unsigned int serviceReqConnectNum = 0;
short unsigned int actionConnectNum = 0;
short unsigned int actionReqConnectNum = 0;


//lists of names and file
char ** pubNames = NULL;
char ** subNames = NULL;
char ** pubFiles = NULL;
char ** serviceNames = NULL;
char ** serviceFiles = NULL;
char ** actionNames = NULL;
char ** actionFiles = NULL;

typedef struct {
	char * name;
	char * masterComm;
} Info;

//frees memory when ctrl-c is hit
void exitHandler(int _){
	printf("\nfreeing publishers\n");
	for(unsigned int i = 0; i < pubConnectNum; ++i){
		free(pubNames[i]);
		free(pubFiles[i]);
	}
	free(pubNames);
	free(pubFiles);
	printf("freeing subscribers\n");
	for(unsigned int i = 0;i<subConnectNum;++i){
		free(subNames[i]);
	}
	free(subNames);
	printf("freeing services\n");
	for(unsigned int i = 0;i<serviceConnectNum;++i){
		free(serviceNames[i]);
		free(serviceFiles[i]);
	}	
	free(serviceNames);
	free(serviceFiles);
	
	printf("freeing action servers\n");
	for(unsigned int i = 0;i<actionConnectNum;++i){
		free(actionNames[i]);
		free(actionFiles[i]);	
	}
	free(actionNames);
	free(actionFiles);
	printf("Ingore this error nanomsg is being stupid... : \n");	
	nn_term();
	exit(1);
}

//0 for pub, 1 for sub, 2 for service, 3 for action
char ** addToList(char ** names, char * nameToAdd, int i){
    if (i == 1){
        names = (char **)realloc(names, (subConnectNum+1)*sizeof(char *));
        char * newString = (char*)malloc((strlen(nameToAdd))*sizeof(char));
        strcpy(newString, nameToAdd);
        names[subConnectNum] = newString;
        return names;
    } else if (i == 2){
        names = (char **)realloc(names, (serviceConnectNum+1)*sizeof(char *));
        char * newString = (char*)malloc((strlen(nameToAdd))*sizeof(char));
        strcpy(newString, nameToAdd);
        names[serviceConnectNum] = newString;
        return names;
	} else if(i == 3){
		names = (char **)realloc(names, (actionConnectNum+1)*sizeof(char *));
        char * newString = (char*)malloc((strlen(nameToAdd))*sizeof(char));
        strcpy(newString, nameToAdd);
        names[actionConnectNum] = newString;
        return names;
	} else {
names = (char **)realloc(names, (pubConnectNum+1)*sizeof(char *));
        char * newString = (char*)malloc((strlen(nameToAdd))*sizeof(char));
        strcpy(newString, nameToAdd);
        names[pubConnectNum] = newString;
        return names;
    }
}

//handles comunication with sub nodes
void * commWithSubNode(void * stuff){
	Info * tmp = (Info *)stuff;
	char name[100];
	strcpy(name, tmp[0].name); 
	char fileName[9];
	strcpy(fileName, tmp[0].masterComm);
	char addrToBind[23] = "ipc:///tmp/";
	strcat(addrToBind, fileName);
	
	int S2 = nn_socket(AF_SP, NN_PAIR);
	if ((nn_bind(S2, addrToBind)) < 0){
		fprintf(stderr, "error: failed to instantiate pair to subscriber\n%s\n", strerror(errno));
	}
	//sends files to sub to to sub node
	unsigned int i = 0;
	while(1){
		if(pubNames != NULL){
			//dont want to send the same info over and over again
			for(;i<pubConnectNum;++i){
				if(pubNames[i] != NULL){
					if(strcmp(pubNames[i], name) == 0){
						if ((nn_send(S2, pubFiles[i], 9, 0)) < 0){
							fprintf(stderr, "Error: failed to send file name to subscriber\n%s\n", strerror(errno));
						}
					}
				}
				
			}
		}
		sleep(1);
	}

	return NULL;
}


//handles comm with service reqesters
//effectively the same as sub nodes
void * commWithServiceReq(void * stuff){
	Info * tmp = (Info *)stuff;
    char name[100];
    strcpy(name, tmp[0].name);
    char fileName[9];
    strcpy(fileName, tmp[0].masterComm);
    char addrToBind[23] = "ipc:///tmp/";
    strcat(addrToBind, fileName);
    free(tmp[0].name);
    free(tmp[0].masterComm);
    free(stuff);

    int S2 = nn_socket(AF_SP, NN_PAIR);
    if ((nn_bind(S2, addrToBind)) < 0){
        fprintf(stderr, "error: failed to instantiate pair to server requester\n%s\n", strerror(errno));
    }
	//sends first matching file over to node
    unsigned int i = 0;
    while(1){
        if(serviceNames != NULL){
            //dont want to send the same info over and over again
            for(;i<serviceConnectNum;++i){
                if(serviceNames[i] != NULL){
                    if(strcmp(serviceNames[i], name) == 0){
                        if ((nn_send(S2, serviceFiles[i], 9, 0)) < 0){
                            fprintf(stderr, "Error: failed to send file name to requester\n%s\n", strerror(errno));
                        }
						break;
                    }
                }
            }
        }
    }
	return NULL;
}

void * commWithActionReq(void * stuff){
	Info * tmp = (Info *)stuff;
    char name[100];
    strcpy(name, tmp[0].name);
    char fileName[9];
    strcpy(fileName, tmp[0].masterComm);
    char addrToBind[23] = "ipc:///tmp/";
    strcat(addrToBind, fileName);
    free(tmp[0].name);
    free(tmp[0].masterComm);
    free(stuff);

    int S2 = nn_socket(AF_SP, NN_PAIR);
    if ((nn_bind(S2, addrToBind)) < 0){
        fprintf(stderr, "error: failed to instantiate pair to action requester\n%s\n", strerror(errno));
    }
    //sends first matching file over to node
    unsigned int i = 0;
    while(1){
        if(actionNames != NULL){
            //dont want to send the same info over and over again
            for(;i<actionConnectNum;++i){
                if(actionNames[i] != NULL){
                    if(strcmp(actionNames[i], name) == 0){
                        if ((nn_send(S2, actionFiles[i], 9, 0)) < 0){
                            fprintf(stderr, "Error: failed to send file name to requester\n%s\n", strerror(errno));
                        }
                        break;
                    }
                }
            }
        }
    }
	nn_close(S2);
	return NULL;
}

void * listenForSubNode(void * garboge){
	//listen for nodes and spin up new thread to handle them
	int S1 = nn_socket(AF_SP, NN_REP);
	if ((nn_bind(S1, "ipc:///tmp/masterSub.ipc")) < 0){
		fprintf(stderr, "Error: failed to bind to ipc file\n%s\n", strerror(errno));
		exit(1);
	}	
	pthread_t Tsid;
	while(1){
		char recvBuff[100];
		if ((nn_recv(S1, recvBuff, 100, 0)) < 0){
			fprintf(stderr, "Error: failed to recieve data\n%s\n", strerror(errno));
			exit(1);
		} else {
			//set up comm with subscriber
			subNames = addToList(subNames, recvBuff, 1);
			printf("%s on sub list: %d\n", recvBuff, totalConnectNum);
			//identifier for sub node
			// the following code is what determins the node limit for each type
	        char intToStr[9];
    	    snprintf(intToStr, 5,"%d", totalConnectNum);
        	strcat(intToStr, ".ipc");
			if ((nn_send(S1, intToStr, 9, 0)) < 0){	
				fprintf(stderr,"Error: failed to send name of icp to sub node\n%s\n", strerror(errno));
				exit(1);
			}
			++totalConnectNum;
            ++subConnectNum;
			char tmp[100];
			strcpy(tmp, recvBuff);
			Info * tmpInfo = (Info *)malloc(sizeof(Info));
			//-----------------------------------------------
			tmpInfo[0].name = (char*)malloc(strlen(tmp)*sizeof(char)); 
			tmpInfo[0].masterComm = (char *)malloc(strlen(intToStr)*sizeof(char));
			strcpy(tmpInfo[0].name, tmp);
			strcpy(tmpInfo[0].masterComm, intToStr);
			pthread_create(&Tsid, NULL, commWithSubNode, (void *)tmpInfo);
		}
	}


	nn_close(S1);
	return NULL;
}

void * listenForPubNode(void * garboge){
	//listen for nodes and spin up new thread to handle them
    int S1 = nn_socket(AF_SP, NN_REP);
    if ((nn_bind(S1, "ipc:///tmp/masterPub.ipc")) < 0){
        fprintf(stderr, "Error: failed to bind to ipc file\n%s\n", strerror(errno));
        exit(1);
    }
    pthread_t Tpid;
    while(1){
        char recvBuff[100];
        if ((nn_recv(S1, recvBuff, 100, 0)) < 0){
            fprintf(stderr, "Error: failed to recieve data\n%s\n", strerror(errno));
            exit(1);
        }
		//needs to send return name that was sent || keep track of the number file
        pubNames = addToList(pubNames, recvBuff, 0);
		char intToStr[9];
        snprintf(intToStr, 5,"%d", totalConnectNum);
        strcat(intToStr, ".ipc");
		pubFiles = addToList(pubFiles, intToStr, 0);
        if ((nn_send(S1, intToStr, 9, 0)) < 0){
            fprintf(stderr,"Error: failed to send name of icp to sub node\n%s\n", strerror(errno));
            exit(1);
        }
        printf("%s on pub list: %d\n", recvBuff, totalConnectNum);
		++totalConnectNum;
		++pubConnectNum;
    }
    nn_close(S1);
    return NULL;
}

void * listenForService(void * stuff){
	//listen for nodes and spin up new thread to handle them
    int S1 = nn_socket(AF_SP, NN_REP);
    if ((nn_bind(S1, "ipc:///tmp/masterService.ipc")) < 0){
        fprintf(stderr, "Error: failed to bind to ipc service file\n%s\n", strerror(errno));
        exit(1);
    }
	while(1){
		char recvBuff[100];
        if ((nn_recv(S1, recvBuff, 100, 0)) < 0){
            fprintf(stderr, "Error: failed to recieve data\n%s\n", strerror(errno));
            exit(1);
        }
        serviceNames = addToList(serviceNames, recvBuff, 2);
        char intToStr[9];
        snprintf(intToStr, 5,"%d", totalConnectNum);
        strcat(intToStr, ".ipc");
        serviceFiles = addToList(serviceFiles, intToStr, 2);
        if ((nn_send(S1, intToStr, 9, 0)) < 0){
            fprintf(stderr,"Error: failed to send name of icp to service\n%s\n", strerror(errno));
            exit(1);
        }
        printf("%s on service list: %d\n", recvBuff, totalConnectNum);
        ++totalConnectNum;
        ++serviceConnectNum;
    }
    nn_close(S1);
	return NULL;
}

void * listenForServiceReq(void * stuff){
	//listen for nodes and spin up new thread to handle them
	int S1 = nn_socket(AF_SP, NN_REP);
    if ((nn_bind(S1, "ipc:///tmp/masterServiceReq.ipc")) < 0){
        fprintf(stderr, "Error: failed to bind to ipc file\n%s\n", strerror(errno));
        exit(1);
    }
    pthread_t TAid;
    while(1){
        char recvBuff[100];
        if ((nn_recv(S1, recvBuff, 100, 0)) < 0){
            fprintf(stderr, "Error: failed to recieve data\n%s\n", strerror(errno));
            exit(1);
        } else {
            //set up comm with subscriber
            printf("%s on service request list: %d\n", recvBuff, totalConnectNum);
            //identifier for sub node
            char intToStr[9];
            snprintf(intToStr, 5,"%d", totalConnectNum);
            strcat(intToStr, ".ipc");
            if ((nn_send(S1, intToStr, 9, 0)) < 0){
                fprintf(stderr,"Error: failed to send name of icp to service requester\n%s\n", strerror(errno));
                exit(1);
            }
			++totalConnectNum;
            ++serviceReqConnectNum;

            char tmp[100];
            strcpy(tmp, recvBuff);
            Info * tmpInfo = (Info *)malloc(sizeof(Info));
            //-----------------------------------------------
            //trying to copy a string to a pointer
            tmpInfo[0].name = (char*)malloc(strlen(tmp)*sizeof(char));
            tmpInfo[0].masterComm = (char *)malloc(strlen(intToStr)*sizeof(char));
            strcpy(tmpInfo[0].name, tmp);
			strcpy(tmpInfo[0].masterComm, intToStr);
            pthread_create(&TAid, NULL, commWithServiceReq, (void *)tmpInfo);
        }
    }
    nn_close(S1);
	return NULL;
}

void * listenForAction(void * stuff){
    //listen for nodes
    int S1 = nn_socket(AF_SP, NN_REP);
    short int N1 = nn_bind(S1, "ipc:///tmp/masterAction.ipc");
    if (N1 < 0){
        fprintf(stderr, "Error: failed to bind to ipc action file\n%s\n", strerror(errno));
        exit(1);
    }
    while(1){
        char recvBuff[100];
        if ((nn_recv(S1, recvBuff, 100, 0)) < 0){
            fprintf(stderr, "Error: failed to recieve data\n%s\n", strerror(errno));
            exit(1);
        }
        actionNames = addToList(actionNames, recvBuff, 3);
        char intToStr[9];
        snprintf(intToStr, 5,"%d", totalConnectNum);
        strcat(intToStr, ".ipc");
        actionFiles = addToList(actionFiles, intToStr, 3);
        if ((nn_send(S1, intToStr, 9, 0)) < 0){
            fprintf(stderr,"Error: failed to send name of icp to service\n%s\n", strerror(errno));
            exit(1);
        }
        printf("%s on action server list: %d\n", recvBuff, totalConnectNum);
        ++totalConnectNum;
        ++actionConnectNum;
    }
    nn_close(S1);
	return NULL;
}

void * listenForActionReq(void * stuff){
	//listen for nodes and spin up new thread to handle them
    int S1 = nn_socket(AF_SP, NN_REP);
    if ((nn_bind(S1, "ipc:///tmp/masterActionReq.ipc")) < 0){
        fprintf(stderr, "Error: failed to bind to ipc file\n%s\n", strerror(errno));
        exit(1);
    }
    pthread_t TAid;
    while(1){
        char recvBuff[100];
        if ((nn_recv(S1, recvBuff, 100, 0)) < 0){
            fprintf(stderr, "Error: failed to recieve data\n%s\n", strerror(errno));
            exit(1);
        } else {
            //set up comm with subscriber
            printf("%s sent action request: %d\n", recvBuff, totalConnectNum);
            //identifier for sub node
            char intToStr[9];
            snprintf(intToStr, 5,"%d", totalConnectNum);
            strcat(intToStr, ".ipc");
            int N2 = nn_send(S1, intToStr, 9, 0);
            if ((nn_send(S1, intToStr, 9, 0)) < 0){
                fprintf(stderr,"Error: failed to send name of icp to service requester\n%s\n", strerror(errno));
                exit(1);
            }
			++totalConnectNum;
            ++actionReqConnectNum;
            char tmp[100];
            strcpy(tmp, recvBuff);
            Info * tmpInfo = (Info *)malloc(sizeof(Info));
            //-----------------------------------------------
            //trying to copy a string to a pointer
			tmpInfo[0].name = (char*)malloc(strlen(tmp)*sizeof(char));
            tmpInfo[0].masterComm = (char *)malloc(strlen(intToStr)*sizeof(char));
            strcpy(tmpInfo[0].name, tmp);
            strcpy(tmpInfo[0].masterComm, intToStr);
            pthread_create(&TAid, NULL, commWithActionReq, (void *)tmpInfo);
        }
    }
    nn_close(S1);
	return NULL;
}

int main(){
	signal(SIGINT, exitHandler);
	//spins up all the threads for listening for nodes
    pthread_t subNode;
    pthread_t pubNode;
    pthread_t service;
    pthread_t reqService;
    pthread_t action;
    pthread_t actionReq;


    pthread_create(&action, NULL, listenForAction, NULL);
    pthread_create(&actionReq, NULL, listenForActionReq, NULL);
    pthread_create(&reqService, NULL, listenForServiceReq, NULL);
    pthread_create(&service, NULL, listenForService, NULL);
    pthread_create(&pubNode, NULL, listenForPubNode, NULL);
    pthread_create(&subNode, NULL, listenForSubNode, NULL);
    pthread_join(subNode, NULL);

	return 0;
}

