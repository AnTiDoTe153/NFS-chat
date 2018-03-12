#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include<string.h>
#include<ctype.h>
struct clientInfo 
{
	int sockNumber;
	char ip[INET_ADDRSTRLEN];
	char name[20];
    char pass[20];
};
struct user{
    char name[20];
    char pass[20];
};

struct user user_data;
struct clientInfo clients[100];
struct user users[30];
int n = 0;
int nrOfUsers;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void readUsers(char *fileName)
{
	FILE *f;
    char *line =NULL;
    size_t len = 0;
    ssize_t read;
    char *p;
	f=fopen(fileName, "rt");
	if(!f)
	{

		perror("Nu s-a putut deschide fisierul");
		exit(EXIT_FAILURE);
	}
	while ((read = getline(&line, &len, f)) != -1) 
	{
	p = strtok(line, " ");
    	if(p)
    	{
	  		strcpy(users[nrOfUsers].name,p);
    	}
    	p = strtok(NULL, " ");
        if(p)
        {
           p[strlen(p)-1] ='\0';  //delete new line char		
           strcpy(users[nrOfUsers].pass,p);
        }
        nrOfUsers++;
	}
      
	fclose(f);
}

int verifyUser(char *name,char *pass)
{
	int i,flag=0;
	char str[255];
	strcpy(str,name);
	for(int i = 0; str[i]; i++)
	{
  	    str[i] = tolower(str[i]);
	}
	
	for(i=0;i<nrOfUsers;i++){
	   if(strcmp(str,users[i].name)==0)
	   {
			flag=1;
			if(strcmp(pass,users[i].pass)==0)
			{
			   return 1;  //connected
			}
			else
			{
			   return 0;  //wrong pass
			}
	   }
	}
	if(!flag){
	    return 2;  //username not found
	}
}

int check_login(int their_sock){
	char ans[200];
	int result;

	recv(their_sock,&user_data,sizeof(struct user),0);
	if((result = verifyUser(user_data.name, user_data.pass))){
		strcpy(ans, "accepted");
	}else{
		strcpy(ans, "declined");
	}
	write(their_sock,ans,200);
	return result;
}

void appendUsername(char *msg, char *username){
	char final_msg[200];

	strcpy(final_msg, username);
	strcat(final_msg, ": ");
	strcat(final_msg, msg);

	strcpy(msg, final_msg);
}

void sendMessageToAll(char *msg,struct clientInfo currentClient)
{
	int i;

	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++) 
	{
		if(clients[i].sockNumber != currentClient.sockNumber) 
		{
			if(send(clients[i].sockNumber,msg,strlen(msg),0) < 0) 
			{
				perror("Sending failure!");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}
void *receiveMessage(void *sock)
{
	struct clientInfo cl = *((struct clientInfo *)sock);
	char msg[500];
	int len,i,j;
	strcpy(msg,"");
	while((len = recv(cl.sockNumber,msg,500,0)) > 0) 
	{
		
		appendUsername(msg, cl.name);
		sendMessageToAll(msg,cl);

		memset(msg,'\0',sizeof(msg));
	}
	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n",cl.ip);
	for(i = 0; i < n; i++) 
	{
		if(clients[i].sockNumber == cl.sockNumber) 
		{
			j = i;
			while(j < n-1) 
			{
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	n--;
	pthread_mutex_unlock(&mutex);
}



int initializeSocket(char *portNumber)
{
	struct sockaddr_in myAddress;
	int portno = atoi(portNumber);
	int my_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(myAddress.sin_zero,'\0',sizeof(myAddress.sin_zero));

	myAddress.sin_family = AF_INET;
	myAddress.sin_port = htons(portno);
	myAddress.sin_addr.s_addr = INADDR_ANY;
	
	if(bind(my_sock,(struct sockaddr *)&myAddress,sizeof(myAddress)) != 0)  //ii zici la socket portul si adresele de la care sa asculte
	{ 
		perror("Binding unsuccessful");
		exit(1);
	}

	if(listen(my_sock,5) != 0) //ii zice la socket ca asculta la port si limiteaza la 5 nr de useri care se coencteaza simultam 
	{
		perror("Listening unsuccessful");
		exit(1);
	}
	return my_sock;

}
int main(int argc,char *argv[])
{
	struct sockaddr_in theirAddress;
	socklen_t theirAddressSize = sizeof(theirAddress);
	int mySocket,theirSocket;

	mySocket=initializeSocket(argv[1]);
	struct clientInfo cl;
	char ip[INET_ADDRSTRLEN];
	pthread_t recvt;
	if(argc != 3) 
	{
		printf("too many arguments");
		exit(1);
	}

	readUsers(argv[2]);

	while(1) 
	{
		if((theirSocket = accept(mySocket,(struct sockaddr *)&theirAddress,&theirAddressSize)) < 0)
		{
			perror("Accept unsuccessful");
			exit(1);
		}
		pthread_mutex_lock(&mutex);
		inet_ntop(AF_INET, (struct sockaddr *)&theirAddress, ip, INET_ADDRSTRLEN);

		if(check_login(theirSocket)){

			printf("%s connected\n",ip);
			cl.sockNumber = theirSocket;
			strcpy(cl.ip,ip);
			strcpy(cl.name, user_data.name);

			clients[n].sockNumber = theirSocket;
			strcpy(clients[n].name, user_data.name);
			n++;
			pthread_mutex_unlock(&mutex);
			char msg[200];
			strcpy(msg, user_data.name);
			strcat(msg, " has connected.");
			sendMessageToAll(msg, cl);
			pthread_mutex_lock(&mutex);
			pthread_create(&recvt,NULL,receiveMessage,&cl);
		
		}

		pthread_mutex_unlock(&mutex);
	}
	return 0;
}